#include "XYZ/Engine/RigAnimation.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

#include "XYZ/Engine/Json.h"

namespace xyz::engine {
namespace {

const JsonValue* requiredValue(
    const JsonValue& object,
    std::string_view key,
    const std::string& context,
    std::string& error) {
    const JsonValue* value = object.find(key);
    if (value == nullptr) {
        error = context + " is missing '" + std::string(key) + "'";
    }
    return value;
}

bool readNumber(
    const JsonValue& object,
    std::string_view key,
    const std::string& context,
    float& output,
    std::string& error) {
    const JsonValue* value = requiredValue(object, key, context, error);
    if (value == nullptr) {
        return false;
    }
    if (!value->isNumber() || !std::isfinite(value->number())) {
        error = context + " field '" + std::string(key) + "' must be a finite number";
        return false;
    }
    output = static_cast<float>(value->number());
    if (!std::isfinite(output)) {
        error = context + " field '" + std::string(key) + "' is outside float range";
        return false;
    }
    return true;
}

bool readPoint(
    const JsonValue& object,
    std::string_view key,
    const std::string& context,
    SDL_FPoint& output,
    std::string& error) {
    const JsonValue* value = requiredValue(object, key, context, error);
    if (value == nullptr) {
        return false;
    }
    if (!value->isArray() || value->array().size() != 2
        || !value->array()[0].isNumber() || !value->array()[1].isNumber()
        || !std::isfinite(value->array()[0].number())
        || !std::isfinite(value->array()[1].number())) {
        error = context + " field '" + std::string(key) + "' must be a finite [x, y] array";
        return false;
    }
    output = {
        static_cast<float>(value->array()[0].number()),
        static_cast<float>(value->array()[1].number())};
    if (!std::isfinite(output.x) || !std::isfinite(output.y)) {
        error = context + " field '" + std::string(key) + "' is outside float range";
        return false;
    }
    return true;
}

RigPoseTransform identityTransform() {
    return {};
}

RigPoseTransform interpolate(
    const RigPoseTransform& start,
    const RigPoseTransform& end,
    float amount) {
    return {
        {
            start.positionOffset.x
                + (end.positionOffset.x - start.positionOffset.x) * amount,
            start.positionOffset.y
                + (end.positionOffset.y - start.positionOffset.y) * amount},
        start.rotationDegrees
            + (end.rotationDegrees - start.rotationDegrees) * amount,
        {
            start.scaleMultiplier.x
                + (end.scaleMultiplier.x - start.scaleMultiplier.x) * amount,
            start.scaleMultiplier.y
                + (end.scaleMultiplier.y - start.scaleMultiplier.y) * amount}};
}

SDL_FPoint interpolatePoint(
    const SDL_FPoint& start,
    const SDL_FPoint& end,
    float amount) {
    return {
        start.x + (end.x - start.x) * amount,
        start.y + (end.y - start.y) * amount};
}

RigPose blendPoses(const RigPose& start, const RigPose& end, float amount) {
    RigPose result;
    result.rootOffsetLogical = interpolatePoint(
        start.rootOffsetLogical,
        end.rootOffsetLogical,
        amount);
    std::set<std::string> nodeIds;
    for (const auto& [nodeId, ignored] : start.nodes) {
        static_cast<void>(ignored);
        nodeIds.insert(nodeId);
    }
    for (const auto& [nodeId, ignored] : end.nodes) {
        static_cast<void>(ignored);
        nodeIds.insert(nodeId);
    }
    for (const std::string& nodeId : nodeIds) {
        const auto startIterator = start.nodes.find(nodeId);
        const auto endIterator = end.nodes.find(nodeId);
        const RigPoseTransform startTransform =
            startIterator == start.nodes.end() ? identityTransform() : startIterator->second;
        const RigPoseTransform endTransform =
            endIterator == end.nodes.end() ? identityTransform() : endIterator->second;
        result.nodes.emplace(nodeId, interpolate(startTransform, endTransform, amount));
    }
    return result;
}

} // namespace

bool RigAnimation::load(const std::filesystem::path& path, std::string& error) {
    const auto document = JsonValue::parseFile(path, error);
    if (!document.has_value() || !document->isObject()) {
        if (error.empty()) {
            error = path.string() + " must contain a JSON object";
        }
        return false;
    }

    const std::string context = path.string();
    const JsonValue* nameValue = requiredValue(*document, "name", context, error);
    const JsonValue* loopValue = requiredValue(*document, "loop", context, error);
    const JsonValue* keyframeValues = requiredValue(*document, "keyframes", context, error);
    if (nameValue == nullptr || loopValue == nullptr || keyframeValues == nullptr) {
        return false;
    }
    if (!nameValue->isString() || nameValue->string().empty()) {
        error = context + " field 'name' must be a non-empty string";
        return false;
    }
    if (!loopValue->isBoolean()) {
        error = context + " field 'loop' must be a boolean";
        return false;
    }

    float loadedStrideDistance = 0.0F;
    float loadedDurationSeconds = 0.0F;
    if (!readNumber(*document, "stride_distance", context, loadedStrideDistance, error)
        || !readNumber(*document, "duration_seconds", context, loadedDurationSeconds, error)) {
        return false;
    }
    if (loadedStrideDistance <= 0.0F || loadedDurationSeconds <= 0.0F) {
        error = context + " requires positive stride_distance and duration_seconds";
        return false;
    }
    if (!keyframeValues->isArray() || keyframeValues->array().empty()) {
        error = context + " field 'keyframes' must be a non-empty array";
        return false;
    }

    std::vector<RigAnimationKeyframe> loadedKeyframes;
    loadedKeyframes.reserve(keyframeValues->array().size());
    float previousPhase = -1.0F;
    for (std::size_t index = 0; index < keyframeValues->array().size(); ++index) {
        const JsonValue& keyframeValue = keyframeValues->array()[index];
        const std::string keyframeContext =
            context + " keyframes[" + std::to_string(index) + "]";
        if (!keyframeValue.isObject()) {
            error = keyframeContext + " must be an object";
            return false;
        }

        RigAnimationKeyframe keyframe;
        keyframe.label = "pose_" + std::to_string(index + 1U);
        if (const JsonValue* labelValue = keyframeValue.find("label"); labelValue != nullptr) {
            if (!labelValue->isString() || labelValue->string().empty()) {
                error = keyframeContext + " field 'label' must be a non-empty string";
                return false;
            }
            keyframe.label = labelValue->string();
        }
        if (!readNumber(keyframeValue, "phase", keyframeContext, keyframe.phase, error)) {
            return false;
        }
        if (keyframe.phase < 0.0F || keyframe.phase >= 1.0F
            || keyframe.phase <= previousPhase) {
            error = keyframeContext + " phase must be strictly increasing in [0, 1)";
            return false;
        }
        if (index == 0 && keyframe.phase != 0.0F) {
            error = keyframeContext + " first phase must be 0";
            return false;
        }
        previousPhase = keyframe.phase;

        const JsonValue* nodeValues = requiredValue(
            keyframeValue,
            "nodes",
            keyframeContext,
            error);
        if (nodeValues == nullptr || !nodeValues->isObject()) {
            if (error.empty()) {
                error = keyframeContext + " field 'nodes' must be an object";
            }
            return false;
        }

        if (const JsonValue* rootOffsetValue = keyframeValue.find("root_offset_px");
            rootOffsetValue != nullptr
            && !readPoint(
                keyframeValue,
                "root_offset_px",
                keyframeContext,
                keyframe.pose.rootOffsetLogical,
                error)) {
            return false;
        }

        for (const auto& [nodeId, nodeValue] : nodeValues->object()) {
            const std::string nodeContext = keyframeContext + " node '" + nodeId + "'";
            if (!nodeValue.isObject()) {
                error = nodeContext + " must be an object";
                return false;
            }
            RigPoseTransform transform;
            if (!readPoint(nodeValue, "position", nodeContext, transform.positionOffset, error)
                || !readNumber(nodeValue, "rotation", nodeContext, transform.rotationDegrees, error)
                || !readPoint(nodeValue, "scale", nodeContext, transform.scaleMultiplier, error)) {
                return false;
            }
            if (transform.scaleMultiplier.x <= 0.0F || transform.scaleMultiplier.y <= 0.0F) {
                error = nodeContext + " scale must be positive";
                return false;
            }
            keyframe.pose.nodes.emplace(nodeId, transform);
        }
        loadedKeyframes.push_back(std::move(keyframe));
    }

    name_ = nameValue->string();
    loop_ = loopValue->boolean();
    strideDistance_ = loadedStrideDistance;
    durationSeconds_ = loadedDurationSeconds;
    keyframes_ = std::move(loadedKeyframes);
    return true;
}

RigPose RigAnimation::sample(float normalizedPhase) const {
    RigPose result;
    if (keyframes_.empty()) {
        return result;
    }
    if (keyframes_.size() == 1) {
        return keyframes_.front().pose;
    }

    float phase = normalizedPhase;
    if (loop_) {
        phase = std::fmod(phase, 1.0F);
        if (phase < 0.0F) {
            phase += 1.0F;
        }
    } else {
        phase = std::clamp(phase, 0.0F, 1.0F);
    }

    const auto nextIterator = std::upper_bound(
        keyframes_.begin(),
        keyframes_.end(),
        phase,
        [](float value, const RigAnimationKeyframe& keyframe) {
            return value < keyframe.phase;
        });

    const RigAnimationKeyframe* start = nullptr;
    const RigAnimationKeyframe* end = nullptr;
    float startPhase = 0.0F;
    float endPhase = 1.0F;
    if (nextIterator == keyframes_.begin()) {
        start = &keyframes_.back();
        end = &keyframes_.front();
        startPhase = start->phase;
        endPhase = 1.0F;
    } else if (nextIterator == keyframes_.end()) {
        start = &keyframes_.back();
        end = &keyframes_.front();
        startPhase = start->phase;
        endPhase = 1.0F;
    } else {
        start = &*(nextIterator - 1);
        end = &*nextIterator;
        startPhase = start->phase;
        endPhase = end->phase;
    }

    if (phase < startPhase) {
        phase += 1.0F;
    }
    const float span = std::max(0.000001F, endPhase - startPhase);
    const float amount = std::clamp((phase - startPhase) / span, 0.0F, 1.0F);

    result.rootOffsetLogical = interpolatePoint(
        start->pose.rootOffsetLogical,
        end->pose.rootOffsetLogical,
        amount);

    std::set<std::string> nodeIds;
    for (const auto& [nodeId, ignored] : start->pose.nodes) {
        static_cast<void>(ignored);
        nodeIds.insert(nodeId);
    }
    for (const auto& [nodeId, ignored] : end->pose.nodes) {
        static_cast<void>(ignored);
        nodeIds.insert(nodeId);
    }
    for (const std::string& nodeId : nodeIds) {
        const auto startIterator = start->pose.nodes.find(nodeId);
        const auto endIterator = end->pose.nodes.find(nodeId);
        const RigPoseTransform startTransform =
            startIterator == start->pose.nodes.end()
                ? identityTransform()
                : startIterator->second;
        const RigPoseTransform endTransform =
            endIterator == end->pose.nodes.end()
                ? identityTransform()
                : endIterator->second;
        result.nodes.emplace(nodeId, interpolate(startTransform, endTransform, amount));
    }
    return result;
}

std::size_t RigAnimation::keyframeCount() const noexcept {
    return keyframes_.size();
}

float RigAnimation::strideDistance() const noexcept {
    return strideDistance_;
}

float RigAnimation::durationSeconds() const noexcept {
    return durationSeconds_;
}

std::string_view RigAnimation::name() const noexcept {
    return name_;
}

bool RigAnimation::loop() const noexcept {
    return loop_;
}

std::string_view RigAnimation::keyframeLabel(std::size_t index) const noexcept {
    if (index >= keyframes_.size()) {
        return {};
    }
    return keyframes_[index].label;
}

float RigAnimation::keyframePhase(std::size_t index) const noexcept {
    if (index >= keyframes_.size()) {
        return 0.0F;
    }
    return keyframes_[index].phase;
}

std::size_t RigAnimation::keyframeIndexForPhase(float normalizedPhase) const noexcept {
    if (keyframes_.empty()) {
        return 0;
    }
    float phase = std::fmod(normalizedPhase, 1.0F);
    if (phase < 0.0F) {
        phase += 1.0F;
    }
    const auto next = std::upper_bound(
        keyframes_.begin(),
        keyframes_.end(),
        phase,
        [](float value, const RigAnimationKeyframe& keyframe) {
            return value < keyframe.phase;
        });
    if (next == keyframes_.begin()) {
        return 0;
    }
    if (next == keyframes_.end()) {
        return keyframes_.size() - 1U;
    }
    return static_cast<std::size_t>(std::distance(keyframes_.begin(), next - 1));
}

void RigAnimator::setAnimation(
    const RigAnimation* animation,
    float transitionSeconds,
    bool preservePhase) noexcept {
    if (animation_ == animation) {
        return;
    }

    if (animation_ != nullptr) {
        if (animation_->name() == "walk") {
            walkPhase_ = phase_;
        } else if (animation_->name() == "idle") {
            idlePhase_ = phase_;
        }
    }

    previousPose_ = pose_;
    hasPreviousPose_ = animation_ != nullptr && transitionSeconds > 0.0F;
    transitionDuration_ = hasPreviousPose_ ? transitionSeconds : 0.0F;
    transitionElapsed_ = 0.0F;
    animation_ = animation;
    if (animation_ == nullptr) {
        phase_ = 0.0F;
        pose_ = {};
        keyframeIndex_ = 0;
        hasPreviousPose_ = false;
        return;
    }

    if (preservePhase) {
        phase_ = animation_->name() == "walk" ? walkPhase_ : idlePhase_;
    } else {
        phase_ = 0.0F;
        if (animation_->name() == "walk") {
            walkPhase_ = 0.0F;
        } else if (animation_->name() == "idle") {
            idlePhase_ = 0.0F;
        }
    }
    updatePose();
}

void RigAnimator::reset() noexcept {
    phase_ = 0.0F;
    if (animation_ != nullptr) {
        if (animation_->name() == "walk") {
            walkPhase_ = 0.0F;
        } else if (animation_->name() == "idle") {
            idlePhase_ = 0.0F;
        }
    }
    transitionDuration_ = 0.0F;
    transitionElapsed_ = 0.0F;
    hasPreviousPose_ = false;
    updatePose();
}

void RigAnimator::advanceByDistance(float distance) noexcept {
    if (paused_ || animation_ == nullptr || distance <= 0.0F
        || animation_->strideDistance() <= 0.0F) {
        return;
    }
    const float delta = std::fabs(distance) / animation_->strideDistance();
    phase_ += delta;
    if (animation_->name() == "walk") {
        walkPhase_ = phase_;
    } else {
        idlePhase_ = phase_;
    }
    updatePose();
}

void RigAnimator::advanceByTime(float deltaSeconds) noexcept {
    if (paused_ || animation_ == nullptr || deltaSeconds <= 0.0F
        || animation_->durationSeconds() <= 0.0F) {
        return;
    }
    phase_ += deltaSeconds / animation_->durationSeconds();
    if (animation_->name() == "walk") {
        walkPhase_ = phase_;
    } else {
        idlePhase_ = phase_;
    }
    updatePose();
}

void RigAnimator::updateTransition(float deltaSeconds) noexcept {
    if (!hasPreviousPose_ || transitionDuration_ <= 0.0F) {
        return;
    }
    transitionElapsed_ = std::min(
        transitionElapsed_ + std::max(0.0F, deltaSeconds),
        transitionDuration_);
    updatePose();
}

void RigAnimator::advanceKeyframe(int direction) noexcept {
    if (!paused_ || animation_ == nullptr || animation_->keyframeCount() == 0U
        || direction == 0) {
        return;
    }
    const std::size_t count = animation_->keyframeCount();
    std::size_t index = animation_->keyframeIndexForPhase(phase_);
    if (direction > 0) {
        index = (index + 1U) % count;
    } else {
        index = index == 0U ? count - 1U : index - 1U;
    }
    phase_ = animation_->keyframePhase(index);
    if (animation_->name() == "walk") {
        walkPhase_ = phase_;
    } else {
        idlePhase_ = phase_;
    }
    hasPreviousPose_ = false;
    transitionDuration_ = 0.0F;
    updatePose();
}

void RigAnimator::setPaused(bool paused) noexcept {
    paused_ = paused;
}

const RigPose& RigAnimator::pose() const {
    return pose_;
}

float RigAnimator::phase() const noexcept {
    return phase_;
}

std::string_view RigAnimator::animationName() const noexcept {
    return animation_ == nullptr ? std::string_view{} : animation_->name();
}

std::string_view RigAnimator::keyframeLabel() const noexcept {
    return animation_ == nullptr ? std::string_view{} : animation_->keyframeLabel(keyframeIndex_);
}

std::size_t RigAnimator::keyframeIndex() const noexcept {
    return keyframeIndex_;
}

bool RigAnimator::paused() const noexcept {
    return paused_;
}

bool RigAnimator::isBlending() const noexcept {
    return hasPreviousPose_ && transitionElapsed_ < transitionDuration_;
}

void RigAnimator::updatePose() noexcept {
    if (animation_ == nullptr) {
        pose_ = {};
        phase_ = 0.0F;
        keyframeIndex_ = 0;
        return;
    }
    if (animation_->loop()) {
        phase_ = std::fmod(phase_, 1.0F);
        if (phase_ < 0.0F) {
            phase_ += 1.0F;
        }
    } else {
        phase_ = std::clamp(phase_, 0.0F, 1.0F);
    }
    keyframeIndex_ = animation_->keyframeIndexForPhase(phase_);
    const RigPose sampledPose = animation_->sample(phase_);
    if (hasPreviousPose_ && transitionDuration_ > 0.0F) {
        const float amount = std::clamp(
            transitionElapsed_ / transitionDuration_,
            0.0F,
            1.0F);
        pose_ = blendPoses(previousPose_, sampledPose, amount);
        if (amount >= 1.0F) {
            hasPreviousPose_ = false;
        }
    } else {
        pose_ = sampledPose;
    }
}

}
