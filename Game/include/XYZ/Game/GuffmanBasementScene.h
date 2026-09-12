#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#include "XYZ/Engine/Input.h"
#include "XYZ/Engine/Rig2D.h"
#include "XYZ/Engine/RigAnimation.h"
#include "XYZ/Engine/Scene.h"
#include "XYZ/Engine/Texture.h"
#include "XYZ/Game/FootPlantController.h"
#include "XYZ/Game/PlayerController.h"

namespace xyz::game {

class GuffmanBasementScene final : public engine::Scene {
public:
    static constexpr float FloorClickTop = 385.0F;
    static constexpr float PlayerBaselineY = 455.0F;
    static constexpr float PlayerInitialX = 335.0F;
    static constexpr float PlayerBoundsLeft = 80.0F;
    static constexpr float PlayerBoundsRight = 880.0F;
    static constexpr float PlayerVisibleHeight = 188.0F;

    explicit GuffmanBasementScene(std::filesystem::path projectRoot);

    bool initialize(engine::Renderer2D& renderer, std::string& error) override;
    void update(float deltaSeconds, const engine::Input& input, engine::Renderer2D& renderer) override;
    void render(engine::Renderer2D& renderer) const override;

    [[nodiscard]] float playerX() const noexcept;
    [[nodiscard]] bool isWalking() const noexcept;
    [[nodiscard]] Facing playerFacing() const noexcept;
    [[nodiscard]] bool rigDebugEnabled() const noexcept;
    [[nodiscard]] std::string_view rigReloadError() const noexcept;
    [[nodiscard]] std::size_t rigNodeCount() const noexcept;
    [[nodiscard]] std::size_t walkKeyframeCount() const noexcept;
    [[nodiscard]] std::size_t idleKeyframeCount() const noexcept;
    [[nodiscard]] float walkPhase() const noexcept;
    [[nodiscard]] float playerVelocity() const noexcept;
    [[nodiscard]] float rigHeight() const noexcept;
    [[nodiscard]] float characterHeight() const noexcept;
    [[nodiscard]] bool usesLegacyWalkFrames() const noexcept;
    [[nodiscard]] bool masterReferenceEnabled() const noexcept;
    [[nodiscard]] bool rigPaused() const noexcept;
    [[nodiscard]] std::size_t selectedRigNodeIndex() const noexcept;
    [[nodiscard]] std::string_view selectedRigNodeId() const noexcept;
    [[nodiscard]] std::string_view rigKeyframeLabel() const noexcept;
    [[nodiscard]] std::size_t rigKeyframeIndex() const noexcept;
    [[nodiscard]] FootPlantController::SupportFoot plantedFoot() const noexcept;
    [[nodiscard]] float visualRootCorrectionX() const noexcept;
    [[nodiscard]] bool footPlantCorrectionEnabled() const noexcept;
    [[nodiscard]] std::string_view rigReviewError() const noexcept;

private:
    bool reloadRig(engine::Renderer2D& renderer, std::string& error);
    void applyCalibrationInput(const engine::RigCalibrationInput& calibration);

    std::filesystem::path projectRoot_;
    engine::Texture background_;
    engine::Texture masterReference_;
    engine::Rig2D playerRig_;
    engine::RigAnimation idleRigAnimation_;
    engine::RigAnimation walkRigAnimation_;
    engine::RigAnimator rigAnimator_;
    PlayerController player_;
    bool walking_ = false;
    bool rigDebugEnabled_ = false;
    bool masterReferenceEnabled_ = false;
    bool footPlantEnabled_ = true;
    std::string rigReloadError_;
    std::string rigReviewError_;
    std::size_t selectedRigNodeIndex_ = 0;
    FootPlantController footPlant_;
    float lastWalkPhase_ = 0.0F;
    float fps_ = 0.0F;
    bool initialized_ = false;
};

}
