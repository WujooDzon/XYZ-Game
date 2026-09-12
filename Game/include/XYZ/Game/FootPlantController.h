#pragma once

namespace xyz::game {

class FootPlantController {
public:
    enum class SupportFoot {
        None,
        Near,
        Far
    };

    struct State {
        SupportFoot support = SupportFoot::None;
        float nearFootWorldX = 0.0F;
        float farFootWorldX = 0.0F;
        float plantedWorldX = 0.0F;
        float desiredCorrectionX = 0.0F;
        float rootCorrectionX = 0.0F;
    };

    State update(
        float nearFootX,
        float farFootX,
        float walkPhase,
        bool walking,
        float deltaSeconds) noexcept;
    void reset() noexcept;

    [[nodiscard]] State state() const noexcept;

private:
    static float approach(float current, float target, float maxDelta) noexcept;

    SupportFoot support_ = SupportFoot::None;
    float nearFootWorldX_ = 0.0F;
    float farFootWorldX_ = 0.0F;
    float plantedWorldX_ = 0.0F;
    float desiredCorrectionX_ = 0.0F;
    float rootCorrectionX_ = 0.0F;
};

}
