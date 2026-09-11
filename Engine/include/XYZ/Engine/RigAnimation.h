#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "XYZ/Engine/Rig2D.h"

namespace xyz::engine {

struct RigAnimationKeyframe {
    float phase = 0.0F;
    std::string label;
    RigPose pose;
};

class RigAnimation {
public:
    bool load(const std::filesystem::path& path, std::string& error);

    [[nodiscard]] RigPose sample(float normalizedPhase) const;
    [[nodiscard]] std::size_t keyframeCount() const noexcept;
    [[nodiscard]] float strideDistance() const noexcept;
    [[nodiscard]] float durationSeconds() const noexcept;
    [[nodiscard]] std::string_view name() const noexcept;
    [[nodiscard]] bool loop() const noexcept;
    [[nodiscard]] std::string_view keyframeLabel(std::size_t index) const noexcept;
    [[nodiscard]] float keyframePhase(std::size_t index) const noexcept;
    [[nodiscard]] std::size_t keyframeIndexForPhase(float normalizedPhase) const noexcept;

private:
    std::string name_;
    bool loop_ = true;
    float strideDistance_ = 1.0F;
    float durationSeconds_ = 1.0F;
    std::vector<RigAnimationKeyframe> keyframes_;
};

class RigAnimator {
public:
    void setAnimation(
        const RigAnimation* animation,
        float transitionSeconds = 0.0F,
        bool preservePhase = true) noexcept;
    void reset() noexcept;
    void advanceByDistance(float distance) noexcept;
    void advanceByTime(float deltaSeconds) noexcept;
    void updateTransition(float deltaSeconds) noexcept;
    void advanceKeyframe(int direction) noexcept;
    void setPaused(bool paused) noexcept;

    [[nodiscard]] const RigPose& pose() const;
    [[nodiscard]] float phase() const noexcept;
    [[nodiscard]] std::string_view animationName() const noexcept;
    [[nodiscard]] std::string_view keyframeLabel() const noexcept;
    [[nodiscard]] std::size_t keyframeIndex() const noexcept;
    [[nodiscard]] bool paused() const noexcept;
    [[nodiscard]] bool isBlending() const noexcept;

private:
    void updatePose() noexcept;

    const RigAnimation* animation_ = nullptr;
    RigPose pose_;
    RigPose previousPose_;
    float phase_ = 0.0F;
    float walkPhase_ = 0.0F;
    float idlePhase_ = 0.0F;
    float transitionDuration_ = 0.0F;
    float transitionElapsed_ = 0.0F;
    std::size_t keyframeIndex_ = 0;
    bool hasPreviousPose_ = false;
    bool paused_ = false;
};

}
