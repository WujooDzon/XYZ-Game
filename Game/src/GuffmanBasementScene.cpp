#include "XYZ/Game/GuffmanBasementScene.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

#include "XYZ/Engine/Input.h"
#include "XYZ/Engine/Renderer2D.h"

namespace xyz::game {
namespace {

std::filesystem::path rigDirectory(const std::filesystem::path& root) {
    return root / "Assets" / "Characters" / "Logen" / "Rig";
}

const char* supportFootName(FootPlantController::SupportFoot foot) {
    switch (foot) {
        case FootPlantController::SupportFoot::Left:
            return "left";
        case FootPlantController::SupportFoot::Right:
            return "right";
        case FootPlantController::SupportFoot::None:
            return "none";
    }
    return "none";
}

} // namespace

GuffmanBasementScene::GuffmanBasementScene(std::filesystem::path projectRoot)
    : projectRoot_(std::move(projectRoot)),
      player_({PlayerBoundsLeft, PlayerBoundsRight}, PlayerInitialX, PlayerBaselineY, 220.0F) {}

bool GuffmanBasementScene::initialize(engine::Renderer2D& renderer, std::string& error) {
    const std::filesystem::path backgroundPath =
        projectRoot_ / "Assets" / "Locations" / "GuffmanBasement" / "GuffmanBasement_BG_v1.png";
    const std::filesystem::path masterPath =
        projectRoot_ / "Assets" / "Characters" / "Logen" / "Logen_Master_Right_v1.png";

    if (!background_.load(renderer.native(), backgroundPath, "basement background")) {
        error = "Could not load basement background: " + backgroundPath.string();
        return false;
    }
    if (!masterReference_.load(renderer.native(), masterPath, "Logen master reference")) {
        error = "Could not load Logen master reference: " + masterPath.string();
        return false;
    }
    if (!reloadRig(renderer, error)) {
        return false;
    }

    if (const auto pelvis = playerRig_.nodeIndex("pelvis"); pelvis.has_value()) {
        selectedRigNodeIndex_ = *pelvis;
    }
    playerRig_.setRootPosition({player_.x(), player_.baselineY()});
    playerRig_.setVisualRootCorrectionX(0.0F);
    playerRig_.setMirrored(player_.facing() == Facing::Left);
    footPlant_.reset();
    initialized_ = true;
    return true;
}

bool GuffmanBasementScene::reloadRig(engine::Renderer2D& renderer, std::string& error) {
    const std::filesystem::path directory = rigDirectory(projectRoot_);
    const std::filesystem::path definitionPath = directory / "Logen_rig_definition.json";
    const std::filesystem::path walkPath = directory / "Logen_walk.json";
    const std::filesystem::path idlePath = directory / "Logen_idle.json";

    engine::Rig2D loadedRig;
    engine::RigAnimation loadedWalk;
    engine::RigAnimation loadedIdle;
    if (!loadedRig.loadDefinition(renderer, definitionPath, error)) {
        error = "Could not load Logen rig definition: " + error;
        return false;
    }
    if (!loadedWalk.load(walkPath, error)) {
        error = "Could not load Logen walk animation: " + error;
        return false;
    }
    if (!loadedIdle.load(idlePath, error)) {
        error = "Could not load Logen idle animation: " + error;
        return false;
    }

    const bool wasPaused = rigAnimator_.paused();
    playerRig_ = std::move(loadedRig);
    walkRigAnimation_ = std::move(loadedWalk);
    idleRigAnimation_ = std::move(loadedIdle);
    rigAnimator_.setAnimation(
        walking_ ? &walkRigAnimation_ : &idleRigAnimation_,
        0.0F,
        false);
    rigAnimator_.setPaused(wasPaused);
    selectedRigNodeIndex_ = std::min(
        selectedRigNodeIndex_,
        playerRig_.nodes().empty() ? 0U : playerRig_.nodes().size() - 1U);
    playerRig_.setRootPosition({player_.x(), player_.baselineY()});
    playerRig_.setVisualRootCorrectionX(0.0F);
    playerRig_.setMirrored(player_.facing() == Facing::Left);
    footPlant_.reset();
    lastWalkPhase_ = 0.0F;
    return true;
}

void GuffmanBasementScene::applyCalibrationInput(const engine::RigCalibrationInput& calibration) {
    if (playerRig_.nodes().empty()) {
        return;
    }
    if (calibration.nextNode) {
        selectedRigNodeIndex_ = (selectedRigNodeIndex_ + 1U) % playerRig_.nodes().size();
    }
    if (calibration.previousNode) {
        selectedRigNodeIndex_ = selectedRigNodeIndex_ == 0U
            ? playerRig_.nodes().size() - 1U
            : selectedRigNodeIndex_ - 1U;
    }
    const bool poseStepRequested = calibration.stepPrevious || calibration.stepNext;
    if (poseStepRequested && rigAnimator_.paused()
        && rigAnimator_.animationName() != "walk") {
        rigAnimator_.setAnimation(&walkRigAnimation_, 0.0F, false);
    }
    if (calibration.stepPrevious && rigAnimator_.paused()) {
        rigAnimator_.advanceKeyframe(-1);
    }
    if (calibration.stepNext && rigAnimator_.paused()) {
        rigAnimator_.advanceKeyframe(1);
    }

    const float movementStep = calibration.largeStep ? 10.0F : 1.0F;
    SDL_FPoint movement{0.0F, 0.0F};
    if (calibration.moveLeft) {
        movement.x -= movementStep;
    }
    if (calibration.moveRight) {
        movement.x += movementStep;
    }
    if (calibration.moveUp) {
        movement.y -= movementStep;
    }
    if (calibration.moveDown) {
        movement.y += movementStep;
    }
    if (movement.x != 0.0F || movement.y != 0.0F) {
        static_cast<void>(playerRig_.adjustNodePosition(selectedRigNodeIndex_, movement));
    }

    float rotation = 0.0F;
    if (calibration.rotateLeft) {
        rotation -= calibration.largeStep ? 5.0F : 1.0F;
    }
    if (calibration.rotateRight) {
        rotation += calibration.largeStep ? 5.0F : 1.0F;
    }
    if (rotation != 0.0F) {
        static_cast<void>(playerRig_.adjustNodeRotation(selectedRigNodeIndex_, rotation));
    }

    SDL_FPoint pivot{0.0F, 0.0F};
    if (calibration.pivotLeft) {
        pivot.x -= 0.01F;
    }
    if (calibration.pivotRight) {
        pivot.x += 0.01F;
    }
    if (calibration.pivotUp) {
        pivot.y -= 0.01F;
    }
    if (calibration.pivotDown) {
        pivot.y += 0.01F;
    }
    if (pivot.x != 0.0F || pivot.y != 0.0F) {
        static_cast<void>(playerRig_.adjustNodePivot(selectedRigNodeIndex_, pivot));
    }
}

void GuffmanBasementScene::update(
    float deltaSeconds,
    const engine::Input& input,
    engine::Renderer2D& renderer) {
    if (!initialized_) {
        return;
    }

    if (input.rigDebugTogglePressed()) {
        rigDebugEnabled_ = !rigDebugEnabled_;
    }
    if (input.masterReferenceTogglePressed()) {
        masterReferenceEnabled_ = !masterReferenceEnabled_;
    }
    if (input.rigPauseTogglePressed()) {
        const bool pause = !rigAnimator_.paused();
        rigAnimator_.setPaused(pause);
        if (!pause && !walking_ && rigAnimator_.animationName() == "walk") {
            rigAnimator_.setAnimation(&idleRigAnimation_, 0.18F, true);
        }
    }
    if (rigDebugEnabled_) {
        applyCalibrationInput(input.rigCalibration());
        if (input.rigCalibration().save) {
            std::string saveError;
            if (!playerRig_.saveCalibration(saveError)) {
                rigReloadError_ = std::move(saveError);
            } else {
                rigReloadError_.clear();
            }
        }
        if (input.rigReloadPressed()) {
            std::string reloadError;
            if (!reloadRig(renderer, reloadError)) {
                rigReloadError_ = std::move(reloadError);
            } else {
                rigReloadError_.clear();
            }
        }
    }

    if (input.leftMousePressed()) {
        const SDL_FPoint logicalPoint = renderer.windowToLogical(input.clickPosition());
        if (logicalPoint.y >= FloorClickTop
            && logicalPoint.y <= static_cast<float>(engine::Renderer2D::LogicalHeight)) {
            player_.setMoveTarget(logicalPoint.x);
        }
    }

    const float previousX = player_.x();
    player_.update(deltaSeconds, input.state());

    const bool nowWalking = player_.isMoving();
    if (nowWalking != walking_) {
        if (walking_) {
            lastWalkPhase_ = rigAnimator_.phase();
        }
        walking_ = nowWalking;
        rigAnimator_.setAnimation(
            walking_ ? &walkRigAnimation_ : &idleRigAnimation_,
            walking_ ? 0.12F : 0.18F,
            true);
    }

    if (walking_) {
        rigAnimator_.advanceByDistance(std::fabs(player_.x() - previousX));
        lastWalkPhase_ = rigAnimator_.phase();
    } else {
        rigAnimator_.advanceByTime(deltaSeconds);
    }
    rigAnimator_.updateTransition(deltaSeconds);

    playerRig_.setRootPosition({player_.x(), player_.baselineY()});
    playerRig_.setVisualRootCorrectionX(0.0F);
    playerRig_.setMirrored(player_.facing() == Facing::Left);

    const bool settling = !walking_ && rigAnimator_.isBlending();
    const bool renderWalking = walking_ || settling;
    const float plantPhase = walking_ ? rigAnimator_.phase() : lastWalkPhase_;
    const auto footState = footPlant_.update(
        playerRig_.footContactPosition("left_boot", rigAnimator_.pose()).x,
        playerRig_.footContactPosition("right_boot", rigAnimator_.pose()).x,
        plantPhase,
        renderWalking,
        deltaSeconds);
    playerRig_.setVisualRootCorrectionX(footState.rootCorrectionX);
    if (!renderWalking) {
        playerRig_.setVisualRootCorrectionX(0.0F);
    }

    if (deltaSeconds > 0.0F) {
        fps_ = 1.0F / deltaSeconds;
    }
}

void GuffmanBasementScene::render(engine::Renderer2D& renderer) const {
    if (!initialized_) {
        return;
    }

    const SDL_FRect backgroundDestination{
        0.0F,
        0.0F,
        static_cast<float>(engine::Renderer2D::LogicalWidth),
        static_cast<float>(engine::Renderer2D::LogicalHeight)};
    renderer.drawTexture(background_, backgroundDestination);

    if (masterReferenceEnabled_ && masterReference_.loaded()) {
        const float masterHeight = playerRig_.targetHeight();
        const float masterScale = masterHeight / masterReference_.height();
        const float masterWidth = masterReference_.width() * masterScale;
        const SDL_FRect masterDestination{
            std::round(player_.x()) - masterWidth * 0.5F,
            std::round(player_.baselineY()) - masterHeight,
            masterWidth,
            masterHeight};
        renderer.drawTexture(
            masterReference_,
            masterDestination,
            SDL_Color{255, 255, 255, 90},
            player_.facing() == Facing::Left);
    }

    static_cast<void>(playerRig_.render(renderer, rigAnimator_.pose()));
    if (rigDebugEnabled_) {
        static_cast<void>(playerRig_.debugRender(renderer, rigAnimator_.pose()));
        const auto worlds = playerRig_.worldNodes(rigAnimator_.pose());
        const auto selected = std::find_if(
            worlds.begin(),
            worlds.end(),
            [&](const engine::RigWorldNode& node) {
                return node.nodeIndex == static_cast<int>(selectedRigNodeIndex_);
            });
        if (selected != worlds.end() && selected->bounds.w > 0.0F && selected->bounds.h > 0.0F) {
            renderer.drawDebugRect(selected->bounds, {255, 250, 80, SDL_ALPHA_OPAQUE});
        }
    }

    std::ostringstream playerLine;
    playerLine << "Player X: " << std::fixed << std::setprecision(1) << player_.x();
    std::ostringstream fpsLine;
    fpsLine << "FPS: " << static_cast<int>(fps_ + 0.5F);
    renderer.drawDebugText(12.0F, 12.0F, fpsLine.str());
    renderer.drawDebugText(12.0F, 28.0F, playerLine.str());
    renderer.drawDebugText(12.0F, 44.0F, walking_ ? "Animation: walk" : "Animation: idle");
    renderer.drawDebugText(
        12.0F,
        60.0F,
        player_.facing() == Facing::Left ? "Facing: left" : "Facing: right");

    if (rigDebugEnabled_) {
        const bool debugWalkAnimation = rigAnimator_.animationName() == "walk";
        const std::size_t debugPoseCount = debugWalkAnimation
            ? walkRigAnimation_.keyframeCount()
            : idleRigAnimation_.keyframeCount();
        std::ostringstream phaseLine;
        phaseLine << (debugWalkAnimation ? "Walk phase: " : "Idle phase: ")
                  << std::fixed << std::setprecision(3)
                  << (debugWalkAnimation ? rigAnimator_.phase() : rigAnimator_.phase())
                  << "  pose " << (rigAnimator_.keyframeIndex() + 1U) << "/"
                  << debugPoseCount << " " << rigAnimator_.keyframeLabel();
        renderer.drawDebugText(12.0F, 76.0F, phaseLine.str());

        std::ostringstream velocityLine;
        velocityLine << "Velocity: " << std::fixed << std::setprecision(1)
                     << player_.velocity();
        renderer.drawDebugText(12.0F, 92.0F, velocityLine.str());

        std::ostringstream targetLine;
        targetLine << "Target X: ";
        if (player_.targetX().has_value()) {
            targetLine << std::fixed << std::setprecision(1) << *player_.targetX();
        } else {
            targetLine << "-";
        }
        renderer.drawDebugText(12.0F, 108.0F, targetLine.str());

        std::ostringstream stateLine;
        stateLine << "State: "
                  << (walking_ ? "walk" : rigAnimator_.isBlending() ? "settling" : "idle")
                  << (rigAnimator_.paused() ? " PAUSED" : "");
        renderer.drawDebugText(12.0F, 124.0F, stateLine.str());

        std::ostringstream heightLine;
        heightLine << "Height: " << std::fixed << std::setprecision(1)
                   << playerRig_.neutralHeight() << "/" << playerRig_.targetHeight();
        renderer.drawDebugText(12.0F, 140.0F, heightLine.str());

        std::ostringstream footLine;
        footLine << "Planted foot: " << supportFootName(footPlant_.state().support)
                 << "  visual root X: " << std::fixed << std::setprecision(2)
                 << footPlant_.state().rootCorrectionX;
        renderer.drawDebugText(12.0F, 156.0F, footLine.str());

        std::ostringstream masterLine;
        masterLine << "Master overlay: " << (masterReferenceEnabled_ ? "ON" : "OFF");
        renderer.drawDebugText(12.0F, 172.0F, masterLine.str());

        const auto worlds = playerRig_.worldNodes(rigAnimator_.pose());
        const auto selected = std::find_if(
            worlds.begin(),
            worlds.end(),
            [&](const engine::RigWorldNode& node) {
                return node.nodeIndex == static_cast<int>(selectedRigNodeIndex_);
            });
        const auto& node = playerRig_.nodes()[selectedRigNodeIndex_];
        std::ostringstream selectedLine;
        selectedLine << "Selected: " << node.id << " parent ";
        selectedLine << (node.parentIndex < 0 ? "-" : playerRig_.nodes()[node.parentIndex].id);
        renderer.drawDebugText(12.0F, 188.0F, selectedLine.str());
        std::ostringstream localLine;
        localLine << "Local pos: " << std::fixed << std::setprecision(1)
                  << node.localPosition.x << "," << node.localPosition.y
                  << " pivot " << std::setprecision(2) << node.pivot.x << "," << node.pivot.y;
        renderer.drawDebugText(12.0F, 204.0F, localLine.str());
        std::ostringstream rotationLine;
        rotationLine << "Base rot: " << std::fixed << std::setprecision(1)
                     << node.baseRotationDegrees << " z " << node.zOrder;
        renderer.drawDebugText(12.0F, 220.0F, rotationLine.str());
        if (selected != worlds.end()) {
            std::ostringstream worldLine;
            worldLine << "World: " << std::fixed << std::setprecision(1)
                      << selected->position.x << "," << selected->position.y
                      << " rot " << selected->rotationDegrees;
            renderer.drawDebugText(12.0F, 236.0F, worldLine.str());
        }
        renderer.drawDebugText(
            12.0F,
            252.0F,
            "TAB node  arrows move  Q/E rotate  J/L/I/K pivot  S save  F4 master  F5 pause");
    }

    if (!rigReloadError_.empty()) {
        renderer.drawDebugText(12.0F, 268.0F, "Rig calibration error:");
        renderer.drawDebugText(12.0F, 284.0F, rigReloadError_);
    }
}

float GuffmanBasementScene::playerX() const noexcept {
    return player_.x();
}

bool GuffmanBasementScene::isWalking() const noexcept {
    return walking_;
}

Facing GuffmanBasementScene::playerFacing() const noexcept {
    return player_.facing();
}

bool GuffmanBasementScene::rigDebugEnabled() const noexcept {
    return rigDebugEnabled_;
}

std::string_view GuffmanBasementScene::rigReloadError() const noexcept {
    return rigReloadError_;
}

std::size_t GuffmanBasementScene::rigNodeCount() const noexcept {
    return playerRig_.nodes().size();
}

std::size_t GuffmanBasementScene::walkKeyframeCount() const noexcept {
    return walkRigAnimation_.keyframeCount();
}

std::size_t GuffmanBasementScene::idleKeyframeCount() const noexcept {
    return idleRigAnimation_.keyframeCount();
}

float GuffmanBasementScene::walkPhase() const noexcept {
    return walking_ ? rigAnimator_.phase() : 0.0F;
}

float GuffmanBasementScene::playerVelocity() const noexcept {
    return player_.velocity();
}

float GuffmanBasementScene::rigHeight() const noexcept {
    return characterHeight();
}

float GuffmanBasementScene::characterHeight() const noexcept {
    if (playerRig_.nodes().empty()) {
        return PlayerVisibleHeight;
    }
    return playerRig_.neutralHeight();
}

bool GuffmanBasementScene::usesLegacyWalkFrames() const noexcept {
    return false;
}

bool GuffmanBasementScene::masterReferenceEnabled() const noexcept {
    return masterReferenceEnabled_;
}

bool GuffmanBasementScene::rigPaused() const noexcept {
    return rigAnimator_.paused();
}

std::size_t GuffmanBasementScene::selectedRigNodeIndex() const noexcept {
    return selectedRigNodeIndex_;
}

std::string_view GuffmanBasementScene::selectedRigNodeId() const noexcept {
    if (selectedRigNodeIndex_ >= playerRig_.nodes().size()) {
        return {};
    }
    return playerRig_.nodes()[selectedRigNodeIndex_].id;
}

std::string_view GuffmanBasementScene::rigKeyframeLabel() const noexcept {
    return rigAnimator_.keyframeLabel();
}

std::size_t GuffmanBasementScene::rigKeyframeIndex() const noexcept {
    return rigAnimator_.keyframeIndex();
}

FootPlantController::SupportFoot GuffmanBasementScene::plantedFoot() const noexcept {
    return footPlant_.state().support;
}

float GuffmanBasementScene::visualRootCorrectionX() const noexcept {
    return footPlant_.state().rootCorrectionX;
}

}
