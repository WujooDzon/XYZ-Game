#include "XYZ/Game/GuffmanBasementScene.h"

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

} // namespace

GuffmanBasementScene::GuffmanBasementScene(std::filesystem::path projectRoot)
    : projectRoot_(std::move(projectRoot)),
      player_({PlayerBoundsLeft, PlayerBoundsRight}, PlayerInitialX, PlayerBaselineY, 220.0F) {}

bool GuffmanBasementScene::initialize(engine::Renderer2D& renderer, std::string& error) {
    const std::filesystem::path backgroundPath =
        projectRoot_ / "Assets" / "Locations" / "GuffmanBasement" / "GuffmanBasement_BG_v1.png";

    if (!background_.load(renderer.native(), backgroundPath, "basement background")) {
        error = "Could not load basement background: " + backgroundPath.string();
        return false;
    }

    if (!reloadRig(renderer, error)) {
        return false;
    }

    playerRig_.setRootPosition({player_.x(), player_.baselineY()});
    playerRig_.setMirrored(player_.facing() == Facing::Left);
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

    playerRig_ = std::move(loadedRig);
    walkRigAnimation_ = std::move(loadedWalk);
    idleRigAnimation_ = std::move(loadedIdle);
    rigAnimator_.setAnimation(walking_ ? &walkRigAnimation_ : &idleRigAnimation_);
    playerRig_.setRootPosition({player_.x(), player_.baselineY()});
    // TODO: dedicated left-facing Logen rig required before final production.
    playerRig_.setMirrored(player_.facing() == Facing::Left);
    return true;
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
    if (rigDebugEnabled_ && input.rigReloadPressed()) {
        std::string reloadError;
        if (!reloadRig(renderer, reloadError)) {
            rigReloadError_ = std::move(reloadError);
        } else {
            rigReloadError_.clear();
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
        walking_ = nowWalking;
        rigAnimator_.setAnimation(walking_ ? &walkRigAnimation_ : &idleRigAnimation_);
    }

    if (walking_) {
        rigAnimator_.advanceByDistance(std::fabs(player_.x() - previousX));
    } else {
        rigAnimator_.advanceByTime(deltaSeconds);
    }

    playerRig_.setRootPosition({player_.x(), player_.baselineY()});
    // TODO: dedicated left-facing Logen rig required before final production.
    playerRig_.setMirrored(player_.facing() == Facing::Left);

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

    static_cast<void>(playerRig_.render(renderer, rigAnimator_.pose()));
    if (rigDebugEnabled_) {
        static_cast<void>(playerRig_.debugRender(renderer, rigAnimator_.pose()));
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
        std::ostringstream phaseLine;
        phaseLine << "Rig phase: " << std::fixed << std::setprecision(3)
                  << rigAnimator_.phase();
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

        std::ostringstream heightLine;
        heightLine << "Character height: " << std::fixed << std::setprecision(1)
                   << characterHeight();
        renderer.drawDebugText(12.0F, 124.0F, heightLine.str());
    }

    if (!rigReloadError_.empty()) {
        renderer.drawDebugText(12.0F, 140.0F, "Rig reload error:");
        renderer.drawDebugText(12.0F, 156.0F, rigReloadError_);
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
    if (!initialized_ && playerRig_.nodes().empty()) {
        return PlayerVisibleHeight;
    }
    return playerRig_.bounds(rigAnimator_.pose()).h;
}

bool GuffmanBasementScene::usesLegacyWalkFrames() const noexcept {
    return false;
}

}
