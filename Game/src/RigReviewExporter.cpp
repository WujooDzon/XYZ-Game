#include "XYZ/Game/RigReviewExporter.h"

#include <array>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "XYZ/Engine/Rig2D.h"
#include "XYZ/Engine/RigAnimation.h"
#include "XYZ/Engine/Renderer2D.h"

namespace xyz::game {
namespace {

constexpr float kReviewBaseline = 500.0F;
constexpr float kReviewCenterX = 480.0F;
constexpr float kContactSheetStartX = 72.0F;
constexpr float kContactSheetStepX = 118.0F;

struct ReviewFrame {
    const char* filename;
    const char* label;
};

constexpr std::array<ReviewFrame, 8> kWalkFrames{
    ReviewFrame{"01_contact_a.png", "1/8 CONTACT A"},
    ReviewFrame{"02_down_a.png", "2/8 DOWN A"},
    ReviewFrame{"03_passing_a.png", "3/8 PASSING A"},
    ReviewFrame{"04_up_a.png", "4/8 UP A"},
    ReviewFrame{"05_contact_b.png", "5/8 CONTACT B"},
    ReviewFrame{"06_down_b.png", "6/8 DOWN B"},
    ReviewFrame{"07_passing_b.png", "7/8 PASSING B"},
    ReviewFrame{"08_up_b.png", "8/8 UP B"}};

bool saveCurrentRender(
    SDL_Renderer* renderer,
    const std::filesystem::path& path,
    std::string& error) {
    SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
    if (surface == nullptr) {
        error = "could not read rig review render target: "
            + std::string(SDL_GetError());
        return false;
    }
    const bool saved = IMG_SavePNG(surface, path.string().c_str());
    if (!saved) {
        error = "could not save rig review PNG '" + path.string() + "': "
            + std::string(SDL_GetError());
    }
    SDL_DestroySurface(surface);
    return saved;
}

void clearReviewTarget(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 13, 11, 18, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 104, 87, 86, SDL_ALPHA_OPAQUE);
}

bool drawReviewFrame(
    engine::Renderer2D& renderer,
    engine::Rig2D& rig,
    const engine::RigPose& pose,
    float x,
    std::string_view label) {
    clearReviewTarget(renderer.native());
    rig.setRootPosition({x, kReviewBaseline});
    rig.setVisualRootCorrectionX(0.0F);
    rig.setMirrored(false);
    if (!renderer.drawDebugLine(
            {0.0F, kReviewBaseline + 1.0F},
            {static_cast<float>(engine::Renderer2D::LogicalWidth), kReviewBaseline + 1.0F},
            {104, 87, 86, SDL_ALPHA_OPAQUE})) {
        return false;
    }
    renderer.drawDebugText(16.0F, 16.0F, label);
    return rig.render(renderer, pose);
}

std::string contactSheetLabel(std::size_t index, std::string_view label) {
    std::ostringstream output;
    output << (index + 1U) << " " << label;
    return output.str();
}

} // namespace

bool RigReviewExporter::exportWalkReview(
    engine::Renderer2D& renderer,
    engine::Rig2D& rig,
    const engine::RigAnimation& idleAnimation,
    const engine::RigAnimation& walkAnimation,
    const std::filesystem::path& outputDirectory,
    std::string& error) const {
    error.clear();
    SDL_Renderer* nativeRenderer = renderer.native();
    if (nativeRenderer == nullptr || !renderer.initialized()) {
        error = "cannot export rig review without an initialized renderer";
        return false;
    }
    if (walkAnimation.keyframeCount() != kWalkFrames.size()) {
        error = "cannot export rig review: walk animation must contain exactly eight poses";
        return false;
    }

    std::error_code filesystemError;
    std::filesystem::create_directories(outputDirectory, filesystemError);
    if (filesystemError) {
        error = "could not create rig review directory '" + outputDirectory.string()
            + "': " + filesystemError.message();
        return false;
    }

    SDL_Texture* reviewTarget = SDL_CreateTexture(
        nativeRenderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        engine::Renderer2D::LogicalWidth,
        engine::Renderer2D::LogicalHeight);
    if (reviewTarget == nullptr) {
        error = "could not create rig review render target: "
            + std::string(SDL_GetError());
        return false;
    }

    SDL_Texture* previousTarget = SDL_GetRenderTarget(nativeRenderer);
    const auto finish = [&](bool success) {
        if (!SDL_SetRenderTarget(nativeRenderer, previousTarget) && success) {
            error = "could not restore renderer target after rig review: "
                + std::string(SDL_GetError());
            success = false;
        }
        SDL_DestroyTexture(reviewTarget);
        return success;
    };

    if (!SDL_SetRenderTarget(nativeRenderer, reviewTarget)) {
        error = "could not activate rig review render target: "
            + std::string(SDL_GetError());
        return finish(false);
    }

    if (!drawReviewFrame(
            renderer,
            rig,
            idleAnimation.sample(0.0F),
            kReviewCenterX,
            "00 IDLE")) {
        error = "could not render idle rig review frame: " + std::string(SDL_GetError());
        return finish(false);
    }
    if (!saveCurrentRender(nativeRenderer, outputDirectory / "00_idle.png", error)) {
        return finish(false);
    }

    for (std::size_t index = 0; index < kWalkFrames.size(); ++index) {
        const ReviewFrame& frame = kWalkFrames[index];
        if (!drawReviewFrame(
                renderer,
                rig,
                walkAnimation.sample(walkAnimation.keyframePhase(index)),
                kReviewCenterX,
                frame.label)) {
            error = "could not render walk rig review frame '" + std::string(frame.label)
                + "': " + std::string(SDL_GetError());
            return finish(false);
        }
        if (!saveCurrentRender(nativeRenderer, outputDirectory / frame.filename, error)) {
            return finish(false);
        }
    }

    clearReviewTarget(nativeRenderer);
    if (!renderer.drawDebugLine(
            {0.0F, kReviewBaseline + 1.0F},
            {static_cast<float>(engine::Renderer2D::LogicalWidth), kReviewBaseline + 1.0F},
            {104, 87, 86, SDL_ALPHA_OPAQUE})) {
        error = "could not draw rig review contact-sheet baseline: "
            + std::string(SDL_GetError());
        return finish(false);
    }
    renderer.drawDebugText(16.0F, 16.0F, "LOGEN WALK V3 - CONTACT SHEET");
    for (std::size_t index = 0; index < kWalkFrames.size(); ++index) {
        const ReviewFrame& frame = kWalkFrames[index];
        rig.setRootPosition({kContactSheetStartX + kContactSheetStepX * index, kReviewBaseline});
        rig.setVisualRootCorrectionX(0.0F);
        rig.setMirrored(false);
        const std::string label = contactSheetLabel(index, frame.label + 4);
        renderer.drawDebugText(
            kContactSheetStartX + kContactSheetStepX * index - 42.0F,
            32.0F,
            label);
        if (!rig.render(renderer, walkAnimation.sample(walkAnimation.keyframePhase(index)))) {
            error = "could not render rig review contact sheet: " + std::string(SDL_GetError());
            return finish(false);
        }
    }
    if (!saveCurrentRender(
            nativeRenderer,
            outputDirectory / "Logen_walk_contact_sheet.png",
            error)) {
        return finish(false);
    }

    return finish(true);
}

}
