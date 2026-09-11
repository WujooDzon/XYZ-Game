#include "XYZ/Game/PlayerController.h"

#include <algorithm>
#include <cmath>

namespace xyz::game {

PlayerController::PlayerController(SceneBounds bounds, float initialX, float baselineY, float speed)
    : bounds_(bounds),
      x_(std::clamp(initialX, bounds.left, bounds.right)),
      baselineY_(baselineY),
      speed_(std::max(0.0F, speed)) {}

void PlayerController::setMoveTarget(float targetX) {
    targetX_ = std::clamp(targetX, bounds_.left, bounds_.right);
}

void PlayerController::clearMoveTarget() {
    targetX_.reset();
}

void PlayerController::update(float deltaSeconds, engine::InputState input) {
    const float safeDelta = std::max(0.0F, deltaSeconds);

    const bool manualLeft = input.moveLeft && !input.moveRight;
    const bool manualRight = input.moveRight && !input.moveLeft;
    float desiredVelocity = 0.0F;
    bool hasTarget = false;
    float target = 0.0F;

    if (manualLeft || manualRight) {
        clearMoveTarget();
        const float direction = manualLeft ? -1.0F : 1.0F;
        desiredVelocity = direction * speed_;
        facing_ = manualLeft ? Facing::Left : Facing::Right;
    } else if (targetX_.has_value()) {
        target = *targetX_;
        const float distance = target - x_;
        const float absoluteDistance = std::fabs(distance);
        if (absoluteDistance <= kArrivalTolerance) {
            x_ = target;
            clearMoveTarget();
            velocity_ = 0.0F;
            moving_ = false;
            return;
        }

        const float direction = distance < 0.0F ? -1.0F : 1.0F;
        const float deceleration = speed_ / kDecelerationDuration;
        const float brakingSpeed = deceleration > 0.0F
            ? std::sqrt(2.0F * deceleration * absoluteDistance)
            : 0.0F;
        desiredVelocity = direction * std::min(speed_, brakingSpeed);
        facing_ = direction < 0.0F ? Facing::Left : Facing::Right;
        hasTarget = true;
    }

    const float acceleration = speed_ / kAccelerationDuration;
    const float deceleration = speed_ / kDecelerationDuration;
    const float rate = std::fabs(desiredVelocity) > 0.0F ? acceleration : deceleration;
    velocity_ = approach(velocity_, desiredVelocity, rate * safeDelta);

    const float previousX = x_;
    const float nextX = x_ + velocity_ * safeDelta;

    if (hasTarget) {
        const float distance = target - x_;
        const bool wouldPassTarget = (distance > 0.0F && nextX >= target)
            || (distance < 0.0F && nextX <= target);
        if (wouldPassTarget) {
            x_ = target;
            clearMoveTarget();
            velocity_ = 0.0F;
            moving_ = false;
            return;
        }
    }

    x_ = std::clamp(nextX, bounds_.left, bounds_.right);
    if ((x_ <= bounds_.left && velocity_ < 0.0F)
        || (x_ >= bounds_.right && velocity_ > 0.0F)) {
        velocity_ = 0.0F;
    }
    if (hasTarget && std::fabs(target - x_) <= kArrivalTolerance) {
        x_ = target;
        clearMoveTarget();
        velocity_ = 0.0F;
    }

    const bool hasMovementIntent = manualLeft || manualRight || hasTarget;
    moving_ = std::fabs(x_ - previousX) > 0.0001F
        || std::fabs(velocity_) > 0.0001F
        || (hasMovementIntent && speed_ > 0.0F);
}

float PlayerController::approach(float current, float target, float maxDelta) noexcept {
    if (current < target) {
        return std::min(current + std::max(0.0F, maxDelta), target);
    }
    return std::max(current - std::max(0.0F, maxDelta), target);
}

float PlayerController::x() const noexcept {
    return x_;
}

float PlayerController::baselineY() const noexcept {
    return baselineY_;
}

float PlayerController::velocity() const noexcept {
    return velocity_;
}

std::optional<float> PlayerController::targetX() const noexcept {
    return targetX_;
}

bool PlayerController::isMoving() const noexcept {
    return moving_;
}

Facing PlayerController::facing() const noexcept {
    return facing_;
}

}
