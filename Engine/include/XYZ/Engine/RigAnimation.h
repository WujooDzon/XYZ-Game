#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "XYZ/Engine/Rig2D.h"

namespace xyz::engine {

struct RigAnimationKeyframe {
    float phase = 0.0F;
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

private:
    std::string name_;
    bool loop_ = true;
    float strideDistance_ = 1.0F;
    float durationSeconds_ = 1.0F;
    std::vector<RigAnimationKeyframe> keyframes_;
};

class RigAnimator {
public:
    void setAnimation(const RigAnimation* animation) noexcept;
    void reset() noexcept;
    void advanceByDistance(float distance) noexcept;
    void advanceByTime(float deltaSeconds) noexcept;

    [[nodiscard]] const RigPose& pose() const;
    [[nodiscard]] float phase() const noexcept;
    [[nodiscard]] std::string_view animationName() const noexcept;

private:
    void updatePose() noexcept;

    const RigAnimation* animation_ = nullptr;
    RigPose pose_;
    float phase_ = 0.0F;
};

}
