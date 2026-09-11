#pragma once

#include <optional>

#include "XYZ/Engine/InputState.h"

namespace xyz::game {

struct SceneBounds {
    float left = 0.0F;
    float right = 0.0F;
};

enum class Facing {
    Left,
    Right
};

class PlayerController {
public:
    PlayerController(SceneBounds bounds, float initialX, float baselineY, float speed);

    void setMoveTarget(float targetX);
    void clearMoveTarget();
    void update(float deltaSeconds, engine::InputState input);

    [[nodiscard]] float x() const noexcept;
    [[nodiscard]] float baselineY() const noexcept;
    [[nodiscard]] float velocity() const noexcept;
    [[nodiscard]] std::optional<float> targetX() const noexcept;
    [[nodiscard]] bool isMoving() const noexcept;
    [[nodiscard]] Facing facing() const noexcept;

private:
    static constexpr float kArrivalTolerance = 2.0F;
    static constexpr float kAccelerationDuration = 0.125F;
    static constexpr float kDecelerationDuration = 0.15F;

    static float approach(float current, float target, float maxDelta) noexcept;

    SceneBounds bounds_;
    float x_ = 0.0F;
    float baselineY_ = 0.0F;
    float speed_ = 0.0F;
    float velocity_ = 0.0F;
    std::optional<float> targetX_;
    bool moving_ = false;
    Facing facing_ = Facing::Right;
};

}
