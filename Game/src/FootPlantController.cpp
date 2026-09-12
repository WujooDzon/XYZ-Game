#include "XYZ/Game/FootPlantController.h"

#include <algorithm>
#include <cmath>

namespace xyz::game {
namespace {

constexpr float kTransferSeconds = 0.08F;
constexpr float kMaximumCorrection = 12.0F;

FootPlantController::SupportFoot supportForPhase(float phase) noexcept {
    if (phase < 0.36F) {
        return FootPlantController::SupportFoot::Far;
    }
    if (phase < 0.50F) {
        return FootPlantController::SupportFoot::None;
    }
    if (phase < 0.86F) {
        return FootPlantController::SupportFoot::Near;
    }
    return FootPlantController::SupportFoot::None;
}

}

FootPlantController::State FootPlantController::update(
    float nearFootX,
    float farFootX,
    float walkPhase,
    bool walking,
    float deltaSeconds) noexcept {
    if (!walking) {
        reset();
        return state();
    }

    float normalizedPhase = std::fmod(walkPhase, 1.0F);
    if (normalizedPhase < 0.0F) {
        normalizedPhase += 1.0F;
    }
    nearFootWorldX_ = nearFootX;
    farFootWorldX_ = farFootX;
    const SupportFoot desiredSupport = supportForPhase(normalizedPhase);

    if (support_ != desiredSupport) {
        support_ = desiredSupport;
        if (support_ == SupportFoot::None) {
            plantedWorldX_ = 0.0F;
            desiredCorrectionX_ = 0.0F;
        } else {
            const float newSupportFootX = support_ == SupportFoot::Near
                ? nearFootWorldX_
                : farFootWorldX_;
            plantedWorldX_ = newSupportFootX + rootCorrectionX_;
        }
    }

    if (support_ == SupportFoot::None) {
        desiredCorrectionX_ = 0.0F;
    } else {
        const float supportFootX = support_ == SupportFoot::Near
            ? nearFootWorldX_
            : farFootWorldX_;
        desiredCorrectionX_ = std::clamp(
            plantedWorldX_ - supportFootX,
            -kMaximumCorrection,
            kMaximumCorrection);
    }
    const float safeDelta = std::max(0.0F, deltaSeconds);
    rootCorrectionX_ = approach(
        rootCorrectionX_,
        desiredCorrectionX_,
        kMaximumCorrection * safeDelta / kTransferSeconds);
    return state();
}

void FootPlantController::reset() noexcept {
    support_ = SupportFoot::None;
    nearFootWorldX_ = 0.0F;
    farFootWorldX_ = 0.0F;
    plantedWorldX_ = 0.0F;
    desiredCorrectionX_ = 0.0F;
    rootCorrectionX_ = 0.0F;
}

FootPlantController::State FootPlantController::state() const noexcept {
    return {
        support_,
        nearFootWorldX_,
        farFootWorldX_,
        plantedWorldX_,
        desiredCorrectionX_,
        rootCorrectionX_};
}

float FootPlantController::approach(float current, float target, float maxDelta) noexcept {
    if (current < target) {
        return std::min(current + std::max(0.0F, maxDelta), target);
    }
    return std::max(current - std::max(0.0F, maxDelta), target);
}

}
