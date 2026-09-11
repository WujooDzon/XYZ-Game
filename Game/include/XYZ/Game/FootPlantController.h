#pragma once

namespace xyz::game {

class FootPlantController {
public:
    enum class SupportFoot {
        None,
        Left,
        Right
    };

    struct State {
        SupportFoot support = SupportFoot::None;
        float rootCorrectionX = 0.0F;
    };

    State update(
        float leftFootX,
        float rightFootX,
        float walkPhase,
        bool walking,
        float deltaSeconds) noexcept;
    void reset() noexcept;

    [[nodiscard]] State state() const noexcept;

private:
    static float approach(float current, float target, float maxDelta) noexcept;

    SupportFoot support_ = SupportFoot::None;
    float plantedWorldX_ = 0.0F;
    float rootCorrectionX_ = 0.0F;
};

}
