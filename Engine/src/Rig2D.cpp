#include "XYZ/Engine/Rig2D.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "XYZ/Engine/Json.h"
#include "XYZ/Engine/Renderer2D.h"

namespace xyz::engine {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kTranslationQuantum = 1.0F / 16.0F;

SDL_FPoint add(SDL_FPoint left, SDL_FPoint right) {
    return {left.x + right.x, left.y + right.y};
}

SDL_FPoint multiply(SDL_FPoint left, SDL_FPoint right) {
    return {left.x * right.x, left.y * right.y};
}

SDL_FPoint rotate(SDL_FPoint point, float degrees) {
    const float radians = degrees * kPi / 180.0F;
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return {
        cosine * point.x - sine * point.y,
        sine * point.x + cosine * point.y};
}

float quantize(float value) {
    return std::round(value / kTranslationQuantum) * kTranslationQuantum;
}

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

bool readInteger(
    const JsonValue& object,
    std::string_view key,
    const std::string& context,
    int& output,
    std::string& error) {
    float value = 0.0F;
    if (!readNumber(object, key, context, value, error)) {
        return false;
    }
    if (std::floor(value) != value
        || value < static_cast<float>(std::numeric_limits<int>::min())
        || value > static_cast<float>(std::numeric_limits<int>::max())) {
        error = context + " field '" + std::string(key) + "' must be an integer";
        return false;
    }
    output = static_cast<int>(value);
    return true;
}

void writePoint(std::ostream& output, SDL_FPoint point) {
    output << "[" << point.x << ", " << point.y << "]";
}

} // namespace

bool Rig2D::loadDefinition(
    Renderer2D& renderer,
    const std::filesystem::path& definitionPath,
    std::string& error) {
    error.clear();
    const auto document = JsonValue::parseFile(definitionPath, error);
    if (!document.has_value() || !document->isObject()) {
        if (error.empty()) {
            error = definitionPath.string() + " must contain a JSON object";
        }
        return false;
    }

    DefinitionSettings loadedSettings;
    if (!readNumber(
            *document,
            "global_scale",
            definitionPath.string(),
            loadedSettings.globalScaleMultiplier,
            error)
        || !readNumber(
            *document,
            "target_height",
            definitionPath.string(),
            loadedSettings.targetHeight,
            error)
        || !readPoint(
            *document,
            "root_to_ground",
            definitionPath.string(),
            loadedSettings.rootToGround,
            error)) {
        return false;
    }
    if (loadedSettings.globalScaleMultiplier <= 0.0F
        || loadedSettings.targetHeight <= 0.0F
        || !std::isfinite(loadedSettings.globalScaleMultiplier)
        || !std::isfinite(loadedSettings.targetHeight)) {
        error = definitionPath.string()
            + " requires positive global_scale and target_height";
        return false;
    }

    const JsonValue* nodeValues = requiredValue(
        *document,
        "nodes",
        definitionPath.string(),
        error);
    if (nodeValues == nullptr || !nodeValues->isArray() || nodeValues->array().empty()) {
        if (error.empty()) {
            error = definitionPath.string() + " field 'nodes' must be a non-empty array";
        }
        return false;
    }

    std::vector<RigNode> loadedNodes;
    loadedNodes.reserve(nodeValues->array().size());
    std::unordered_map<std::string, int> indices;
    std::vector<std::string> parentIds;
    int loadedRootIndex = -1;

    for (std::size_t index = 0; index < nodeValues->array().size(); ++index) {
        const JsonValue& value = nodeValues->array()[index];
        const std::string context =
            definitionPath.string() + " node[" + std::to_string(index) + "]";
        if (!value.isObject()) {
            error = context + " must be an object";
            return false;
        }

        const JsonValue* idValue = requiredValue(value, "id", context, error);
        const JsonValue* parentValue = requiredValue(value, "parent", context, error);
        const JsonValue* imageValue = requiredValue(value, "image", context, error);
        if (idValue == nullptr || parentValue == nullptr || imageValue == nullptr) {
            return false;
        }
        if (!idValue->isString() || idValue->string().empty()) {
            error = context + " field 'id' must be a non-empty string";
            return false;
        }
        if (!indices.emplace(idValue->string(), static_cast<int>(index)).second) {
            error = context + " duplicates node id '" + idValue->string() + "'";
            return false;
        }
        if (!parentValue->isNull() && !parentValue->isString()) {
            error = context + " field 'parent' must be null or a string";
            return false;
        }
        if (!imageValue->isString()) {
            error = context + " field 'image' must be a string";
            return false;
        }

        RigNode node;
        node.id = idValue->string();
        node.imagePath = imageValue->string();
        if (!readPoint(value, "position", context, node.localPosition, error)
            || !readPoint(value, "pivot", context, node.pivot, error)
            || !readNumber(value, "rotation", context, node.baseRotationDegrees, error)
            || !readPoint(value, "scale", context, node.localScale, error)
            || !readInteger(value, "z", context, node.zOrder, error)) {
            return false;
        }
        if (value.find("ground_contact") != nullptr
            && !readPoint(value, "ground_contact", context, node.groundContact, error)) {
            return false;
        }
        if (node.pivot.x < 0.0F || node.pivot.x > 1.0F
            || node.pivot.y < 0.0F || node.pivot.y > 1.0F
            || node.groundContact.x < 0.0F || node.groundContact.x > 1.0F
            || node.groundContact.y < 0.0F || node.groundContact.y > 1.0F
            || node.localScale.x <= 0.0F || node.localScale.y <= 0.0F) {
            error = context + " has an invalid pivot, contact anchor, or non-positive scale";
            return false;
        }

        if (parentValue->isNull()) {
            if (loadedRootIndex != -1) {
                error = context + " creates a second root node";
                return false;
            }
            loadedRootIndex = static_cast<int>(index);
            parentIds.emplace_back();
        } else {
            parentIds.push_back(parentValue->string());
        }

        if (!node.imagePath.empty()) {
            const std::filesystem::path resolvedPath =
                definitionPath.parent_path() / std::filesystem::path(node.imagePath);
            if (!node.texture.load(renderer.native(), resolvedPath, node.id.c_str())) {
                error = context + " could not load image '" + resolvedPath.string() + "'";
                return false;
            }
        }
        loadedNodes.push_back(std::move(node));
    }

    if (loadedRootIndex == -1) {
        error = definitionPath.string() + " must contain exactly one root node";
        return false;
    }

    for (std::size_t index = 0; index < loadedNodes.size(); ++index) {
        if (static_cast<int>(index) == loadedRootIndex) {
            continue;
        }
        const auto parentIterator = indices.find(parentIds[index]);
        if (parentIterator == indices.end()) {
            error = definitionPath.string() + " node '" + loadedNodes[index].id
                + "' references unknown parent '" + parentIds[index] + "'";
            return false;
        }
        loadedNodes[index].parentIndex = parentIterator->second;
        loadedNodes[parentIterator->second].children.push_back(static_cast<int>(index));
    }

    std::vector<int> visitState(loadedNodes.size(), 0);
    std::function<bool(int)> visit = [&](int index) {
        if (visitState[index] == 1) {
            error = definitionPath.string() + " contains a parent cycle at node '"
                + loadedNodes[index].id + "'";
            return false;
        }
        visitState[index] = 1;
        for (const int child : loadedNodes[index].children) {
            if (!visit(child)) {
                return false;
            }
        }
        visitState[index] = 2;
        return true;
    };
    if (!visit(loadedRootIndex)) {
        return false;
    }
    for (std::size_t index = 0; index < visitState.size(); ++index) {
        if (visitState[index] == 0) {
            error = definitionPath.string() + " contains a disconnected node '"
                + loadedNodes[index].id + "'";
            return false;
        }
    }

    std::vector<RigNode> previousNodes = std::move(nodes_);
    const int previousRootIndex = rootIndex_;
    const DefinitionSettings previousSettings = settings_;
    const std::filesystem::path previousDefinitionPath = definitionPath_;
    const float previousNeutralHeight = neutralHeight_;
    const float previousEffectiveScale = effectiveScale_;

    nodes_ = std::move(loadedNodes);
    rootIndex_ = loadedRootIndex;
    settings_ = loadedSettings;
    definitionPath_ = definitionPath;
    effectiveScale_ = 1.0F;
    neutralHeight_ = bounds({}).h;
    if (!std::isfinite(neutralHeight_) || neutralHeight_ <= 0.0F) {
        nodes_ = std::move(previousNodes);
        rootIndex_ = previousRootIndex;
        settings_ = previousSettings;
        definitionPath_ = previousDefinitionPath;
        neutralHeight_ = previousNeutralHeight;
        effectiveScale_ = previousEffectiveScale;
        error = definitionPath.string() + " neutral rig has no textured height";
        return false;
    }

    effectiveScale_ = settings_.targetHeight / neutralHeight_
        * settings_.globalScaleMultiplier;
    if (!std::isfinite(effectiveScale_) || effectiveScale_ <= 0.0F) {
        nodes_ = std::move(previousNodes);
        rootIndex_ = previousRootIndex;
        settings_ = previousSettings;
        definitionPath_ = previousDefinitionPath;
        neutralHeight_ = previousNeutralHeight;
        effectiveScale_ = previousEffectiveScale;
        error = definitionPath.string() + " produced an invalid normalized scale";
        return false;
    }
    return true;
}

void Rig2D::setRootPosition(SDL_FPoint baselineAnchor) noexcept {
    gameplayRoot_ = baselineAnchor;
}

void Rig2D::setVisualRootCorrectionX(float correction) noexcept {
    visualRootCorrectionX_ = std::isfinite(correction) ? correction : 0.0F;
}

void Rig2D::setMirrored(bool mirrored) noexcept {
    mirrored_ = mirrored;
}

std::vector<RigWorldNode> Rig2D::worldNodes(const RigPose& pose) const {
    return evaluateWorldNodes(pose);
}

std::vector<RigWorldNode> Rig2D::evaluateWorldNodes(const RigPose& pose) const {
    std::vector<RigWorldNode> output;
    if (rootIndex_ < 0 || rootIndex_ >= static_cast<int>(nodes_.size())) {
        return output;
    }
    output.reserve(nodes_.size());
    const SDL_FPoint rootPosition{
        std::round(gameplayRoot_.x) + quantize(visualRootCorrectionX_)
            - settings_.rootToGround.x * effectiveScale_,
        std::round(gameplayRoot_.y) - settings_.rootToGround.y * effectiveScale_};
    evaluateNode(
        rootIndex_,
        pose,
        rootPosition,
        0.0F,
        {effectiveScale_, effectiveScale_},
        false,
        output);

    if (mirrored_) {
        const float mirrorRootX = rootPosition.x;
        for (RigWorldNode& world : output) {
            world.position.x = quantize(mirrorRootX - (world.position.x - mirrorRootX));
            world.rotationDegrees = -world.rotationDegrees;
            world.pivot.x = 1.0F - world.pivot.x;
        }
    }
    return output;
}

void Rig2D::evaluateNode(
    int nodeIndexValue,
    const RigPose& pose,
    const SDL_FPoint& parentPosition,
    float parentRotationDegrees,
    SDL_FPoint parentScale,
    bool hasParent,
    std::vector<RigWorldNode>& output) const {
    const RigNode& node = nodes_[nodeIndexValue];
    RigPoseTransform poseTransform;
    const auto poseIterator = pose.nodes.find(node.id);
    if (poseIterator != pose.nodes.end()) {
        poseTransform = poseIterator->second;
    }

    const SDL_FPoint localPosition = add(node.localPosition, poseTransform.positionOffset);
    const SDL_FPoint localScale = multiply(node.localScale, poseTransform.scaleMultiplier);
    RigWorldNode world;
    world.nodeIndex = nodeIndexValue;
    world.parentIndex = node.parentIndex;
    world.id = node.id;
    world.pivot = node.pivot;
    world.zOrder = node.zOrder;

    const float localRotation = node.baseRotationDegrees + poseTransform.rotationDegrees;
    if (hasParent) {
        world.position = add(parentPosition, rotate(multiply(localPosition, parentScale), parentRotationDegrees));
        world.scale = multiply(parentScale, localScale);
        world.rotationDegrees = parentRotationDegrees + localRotation;
    } else {
        world.position = add(parentPosition, multiply(localPosition, parentScale));
        world.scale = multiply(parentScale, localScale);
        world.rotationDegrees = localRotation;
    }
    world.position.x = quantize(world.position.x);
    world.position.y = quantize(world.position.y);

    if (node.texture.loaded()) {
        const float width = node.texture.width() * world.scale.x;
        const float height = node.texture.height() * world.scale.y;
        const float pivotX = world.pivot.x * width;
        const float pivotY = world.pivot.y * height;
        const SDL_FPoint corners[] = {
            {-pivotX, -pivotY},
            {width - pivotX, -pivotY},
            {-pivotX, height - pivotY},
            {width - pivotX, height - pivotY}};
        float minimumX = std::numeric_limits<float>::max();
        float minimumY = std::numeric_limits<float>::max();
        float maximumX = std::numeric_limits<float>::lowest();
        float maximumY = std::numeric_limits<float>::lowest();
        for (const SDL_FPoint corner : corners) {
            const SDL_FPoint rotatedCorner = add(world.position, rotate(corner, world.rotationDegrees));
            minimumX = std::min(minimumX, rotatedCorner.x);
            minimumY = std::min(minimumY, rotatedCorner.y);
            maximumX = std::max(maximumX, rotatedCorner.x);
            maximumY = std::max(maximumY, rotatedCorner.y);
        }
        world.bounds = {
            minimumX,
            minimumY,
            maximumX - minimumX,
            maximumY - minimumY};
    }

    output.push_back(world);
    for (const int child : node.children) {
        evaluateNode(
            child,
            pose,
            world.position,
            world.rotationDegrees,
            world.scale,
            true,
            output);
    }
}

bool Rig2D::render(Renderer2D& renderer, const RigPose& pose) const {
    std::vector<RigWorldNode> worlds = evaluateWorldNodes(pose);
    std::sort(worlds.begin(), worlds.end(), [](const RigWorldNode& left, const RigWorldNode& right) {
        if (left.zOrder != right.zOrder) {
            return left.zOrder < right.zOrder;
        }
        return left.nodeIndex < right.nodeIndex;
    });

    bool success = true;
    for (const RigWorldNode& world : worlds) {
        const RigNode& node = nodes_[world.nodeIndex];
        if (!node.texture.loaded()) {
            continue;
        }
        const float width = node.texture.width() * world.scale.x;
        const float height = node.texture.height() * world.scale.y;
        const SDL_FRect destination{
            world.position.x - world.pivot.x * width,
            world.position.y - world.pivot.y * height,
            width,
            height};
        const SDL_FPoint center{
            world.pivot.x * width,
            world.pivot.y * height};
        success = renderer.drawTexture(
                       node.texture,
                       destination,
                       world.rotationDegrees,
                       center,
                       mirrored_)
            && success;
    }
    return success;
}

bool Rig2D::debugRender(Renderer2D& renderer, const RigPose& pose) const {
    const std::vector<RigWorldNode> worlds = evaluateWorldNodes(pose);
    bool success = true;
    for (const RigWorldNode& world : worlds) {
        const SDL_FPoint pivot = world.position;
        success = renderer.drawDebugLine(
                      {pivot.x - 3.0F, pivot.y},
                      {pivot.x + 3.0F, pivot.y},
                      {255, 220, 70, SDL_ALPHA_OPAQUE})
            && success;
        success = renderer.drawDebugLine(
                      {pivot.x, pivot.y - 3.0F},
                      {pivot.x, pivot.y + 3.0F},
                      {255, 220, 70, SDL_ALPHA_OPAQUE})
            && success;
        if (world.bounds.w > 0.0F && world.bounds.h > 0.0F) {
            success = renderer.drawDebugRect(
                          world.bounds,
                          {80, 190, 255, SDL_ALPHA_OPAQUE})
                && success;
        }
        if (world.parentIndex >= 0) {
            const auto parent = std::find_if(
                worlds.begin(),
                worlds.end(),
                [&](const RigWorldNode& candidate) {
                    return candidate.nodeIndex == world.parentIndex;
                });
            if (parent != worlds.end()) {
                success = renderer.drawDebugLine(
                              parent->position,
                              world.position,
                              {255, 120, 80, SDL_ALPHA_OPAQUE})
                    && success;
            }
        }
        renderer.drawDebugText(world.position.x + 5.0F, world.position.y, world.id);
    }
    return success;
}

SDL_FRect Rig2D::bounds(const RigPose& pose) const {
    const std::vector<RigWorldNode> worlds = evaluateWorldNodes(pose);
    SDL_FRect result{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        0.0F,
        0.0F};
    float maximumX = std::numeric_limits<float>::lowest();
    float maximumY = std::numeric_limits<float>::lowest();
    bool hasTexture = false;
    for (const RigWorldNode& world : worlds) {
        if (world.bounds.w <= 0.0F || world.bounds.h <= 0.0F) {
            continue;
        }
        hasTexture = true;
        result.x = std::min(result.x, world.bounds.x);
        result.y = std::min(result.y, world.bounds.y);
        maximumX = std::max(maximumX, world.bounds.x + world.bounds.w);
        maximumY = std::max(maximumY, world.bounds.y + world.bounds.h);
    }
    if (!hasTexture) {
        const SDL_FPoint root{
            std::round(gameplayRoot_.x) + quantize(visualRootCorrectionX_)
                - settings_.rootToGround.x * effectiveScale_,
            std::round(gameplayRoot_.y) - settings_.rootToGround.y * effectiveScale_};
        return {root.x, root.y, 0.0F, 0.0F};
    }
    result.w = maximumX - result.x;
    result.h = maximumY - result.y;
    return result;
}

SDL_FPoint Rig2D::footContactPosition(
    std::string_view nodeId,
    const RigPose& pose) const {
    const auto selected = nodeIndex(nodeId);
    if (!selected.has_value()) {
        return {};
    }
    const std::vector<RigWorldNode> worlds = evaluateWorldNodes(pose);
    const auto worldIterator = std::find_if(
        worlds.begin(),
        worlds.end(),
        [&](const RigWorldNode& world) {
            return world.nodeIndex == static_cast<int>(*selected);
        });
    if (worldIterator == worlds.end()) {
        return {};
    }
    const RigNode& node = nodes_[*selected];
    if (!node.texture.loaded()) {
        return worldIterator->position;
    }
    const float width = node.texture.width() * worldIterator->scale.x;
    const float height = node.texture.height() * worldIterator->scale.y;
    SDL_FPoint localOffset{
        (node.groundContact.x - worldIterator->pivot.x) * width,
        (node.groundContact.y - worldIterator->pivot.y) * height};
    if (mirrored_) {
        localOffset.x = -localOffset.x;
    }
    return add(worldIterator->position, rotate(localOffset, worldIterator->rotationDegrees));
}

const std::vector<RigNode>& Rig2D::nodes() const noexcept {
    return nodes_;
}

std::optional<std::size_t> Rig2D::nodeIndex(std::string_view id) const noexcept {
    for (std::size_t index = 0; index < nodes_.size(); ++index) {
        if (nodes_[index].id == id) {
            return index;
        }
    }
    return std::nullopt;
}

bool Rig2D::adjustNodePosition(std::size_t index, SDL_FPoint delta) noexcept {
    if (index >= nodes_.size() || !std::isfinite(delta.x) || !std::isfinite(delta.y)) {
        return false;
    }
    nodes_[index].localPosition.x += delta.x;
    nodes_[index].localPosition.y += delta.y;
    return std::isfinite(nodes_[index].localPosition.x)
        && std::isfinite(nodes_[index].localPosition.y);
}

bool Rig2D::adjustNodeRotation(std::size_t index, float deltaDegrees) noexcept {
    if (index >= nodes_.size() || !std::isfinite(deltaDegrees)) {
        return false;
    }
    nodes_[index].baseRotationDegrees += deltaDegrees;
    return std::isfinite(nodes_[index].baseRotationDegrees);
}

bool Rig2D::adjustNodePivot(std::size_t index, SDL_FPoint delta) noexcept {
    if (index >= nodes_.size() || !std::isfinite(delta.x) || !std::isfinite(delta.y)) {
        return false;
    }
    nodes_[index].pivot.x = std::clamp(nodes_[index].pivot.x + delta.x, 0.0F, 1.0F);
    nodes_[index].pivot.y = std::clamp(nodes_[index].pivot.y + delta.y, 0.0F, 1.0F);
    return true;
}

bool Rig2D::saveCalibration(std::string& error) const {
    error.clear();
    if (definitionPath_.empty() || nodes_.empty()) {
        error = "cannot save an empty rig calibration";
        return false;
    }

    const std::filesystem::path backupPath = definitionPath_.parent_path()
        / (definitionPath_.stem().string() + ".backup" + definitionPath_.extension().string());
    std::error_code filesystemError;
    std::filesystem::copy_file(
        definitionPath_,
        backupPath,
        std::filesystem::copy_options::overwrite_existing,
        filesystemError);
    if (filesystemError) {
        error = "could not create calibration backup: " + filesystemError.message();
        return false;
    }

    std::ofstream output(definitionPath_, std::ios::trunc);
    if (!output) {
        error = "could not open calibration file for writing: " + definitionPath_.string();
        return false;
    }
    output << std::setprecision(9);
    output << "{\n  \"global_scale\": " << settings_.globalScaleMultiplier
           << ",\n  \"target_height\": " << settings_.targetHeight
           << ",\n  \"root_to_ground\": ";
    writePoint(output, settings_.rootToGround);
    output << ",\n  \"nodes\": [\n";
    for (std::size_t index = 0; index < nodes_.size(); ++index) {
        const RigNode& node = nodes_[index];
        output << "    {\n      \"id\": \"" << node.id << "\",\n      \"parent\": ";
        if (node.parentIndex < 0) {
            output << "null";
        } else {
            output << "\"" << nodes_[node.parentIndex].id << "\"";
        }
        output << ",\n      \"image\": \"" << node.imagePath << "\",\n      \"position\": ";
        writePoint(output, node.localPosition);
        output << ",\n      \"pivot\": ";
        writePoint(output, node.pivot);
        output << ",\n      \"rotation\": " << node.baseRotationDegrees
               << ",\n      \"scale\": ";
        writePoint(output, node.localScale);
        output << ",\n      \"ground_contact\": ";
        writePoint(output, node.groundContact);
        output << ",\n      \"z\": " << node.zOrder << "\n    }"
               << (index + 1U == nodes_.size() ? "\n" : ",\n");
    }
    output << "  ]\n}\n";
    if (!output) {
        error = "could not write calibration file: " + definitionPath_.string();
        return false;
    }
    return true;
}

float Rig2D::globalScale() const noexcept {
    return effectiveScale_;
}

float Rig2D::neutralHeight() const noexcept {
    return neutralHeight_ * effectiveScale_;
}

float Rig2D::targetHeight() const noexcept {
    return settings_.targetHeight;
}

SDL_FPoint Rig2D::rootToGround() const noexcept {
    return settings_.rootToGround;
}

const std::filesystem::path& Rig2D::definitionPath() const noexcept {
    return definitionPath_;
}

bool Rig2D::mirrored() const noexcept {
    return mirrored_;
}

}
