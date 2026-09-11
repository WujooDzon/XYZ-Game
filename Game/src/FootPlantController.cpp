#include "XYZ/Game/FootPlantController.h"

#include <algorithm>
#include <cmath>

namespace xyz::game {
namespace {

constexpr float kTransferSeconds = 0.08F;
constexpr float kMaximumCorrection = 10.0F;

FootPlantController::SupportFoot supportForPhase(float phase) noexcept {
    return phase < 0.5F
        ? FootPlantController::SupportFoot::Left
        : FootPlantController::SupportFoot::Right;
}

}

FootPlantController::State FootPlantController::update(
    float leftFootX,
    float rightFootX,
    float walkPhase,
    bool walking,
    float deltaSeconds) noexcept {
    if (!walking) {
        reset();
        return state();
    }

    const float normalizedPhase = std::fmod(walkPhase, 1.0F) < 0.0F
        ? std::fmod(walkPhase, 1.0F) + 1.0F
        : std::fmod(walkPhase, 1.0F);
    const SupportFoot desiredSupport = supportForPhase(normalizedPhase);
    const float supportFootX = desiredSupport == SupportFoot::Left ? leftFootX : rightFootX;

    if (support_ != desiredSupport) {
        const float currentVisualFootX = support_ == SupportFoot::Left
            ? leftFootX + rootCorrectionX_
            : support_ == SupportFoot::Right
                ? rightFootX + rootCorrectionX_
                : supportFootX;
        plantedWorldX_ = currentVisualFootX;
        support_ = desiredSupport;
    }

    const float desiredCorrection = std::clamp(
        plantedWorldX_ - supportFootX,
        -kMaximumCorrection,
        kMaximumCorrection);
    const float safeDelta = std::max(0.0F, deltaSeconds);
    rootCorrectionX_ = approach(
        rootCorrectionX_,
        desiredCorrection,
        kMaximumCorrection * safeDelta / kTransferSeconds);
    return state();
}

void FootPlantController::reset() noexcept {
    support_ = SupportFoot::None;
    plantedWorldX_ = 0.0F;
    rootCorrectionX_ = 0.0F;
}

FootPlantController::State FootPlantController::state() const noexcept {
    return {support_, rootCorrectionX_};
}

float FootPlantController::approach(float current, float target, float maxDelta) noexcept {
    if (current < target) {
        return std::min(current + std::max(0.0F, maxDelta), target);
    }
    return std::max(current - std::max(0.0F, maxDelta), target);
}

}
