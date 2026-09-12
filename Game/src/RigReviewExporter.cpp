#include "XYZ/Game/RigReviewExporter.h"

#include <array>
#include <filesystem>
#include <fstream>
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

bool createDirectories(const std::filesystem::path& path, std::string& error) {
    std::error_code filesystemError;
    std::filesystem::create_directories(path, filesystemError);
    if (!filesystemError) {
        return true;
    }
    error = "could not create audit directory '" + path.string()
        + "': " + filesystemError.message();
    return false;
}

bool clearTo(SDL_Renderer* renderer, SDL_Color color) {
    return SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE)
        && SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a)
        && SDL_RenderClear(renderer);
}

bool drawAuditPose(
    engine::Renderer2D& renderer,
    engine::Rig2D& rig,
    const engine::RigPose& pose,
    SDL_Color background,
    bool mirrored,
    std::string_view isolatedNode = {}) {
    if (!clearTo(renderer.native(), background)) {
        return false;
    }
    rig.setRootPosition({kReviewCenterX, kReviewBaseline});
    rig.setVisualRootCorrectionX(0.0F);
    rig.setMirrored(mirrored);
    if (!isolatedNode.empty()) {
        return rig.renderNode(renderer, pose, isolatedNode);
    }
    return rig.render(renderer, pose);
}

bool saveTargetPose(
    engine::Renderer2D& renderer,
    engine::Rig2D& rig,
    const engine::RigPose& pose,
    const std::filesystem::path& path,
    SDL_Color background,
    bool mirrored,
    int scale,
    std::string& error,
    std::string_view isolatedNode = {}) {
    SDL_Renderer* nativeRenderer = renderer.native();
    SDL_Texture* logicalTarget = SDL_CreateTexture(
        nativeRenderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        engine::Renderer2D::LogicalWidth,
        engine::Renderer2D::LogicalHeight);
    if (logicalTarget == nullptr) {
        error = "could not create audit render target: " + std::string(SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode(logicalTarget, SDL_SCALEMODE_NEAREST);
    SDL_Texture* previousTarget = SDL_GetRenderTarget(nativeRenderer);
    float previousScaleX = 1.0F;
    float previousScaleY = 1.0F;
    const bool capturedScale = SDL_GetRenderScale(
        nativeRenderer, &previousScaleX, &previousScaleY);
    bool success = capturedScale
        && SDL_SetRenderTarget(nativeRenderer, logicalTarget)
        && SDL_SetRenderScale(nativeRenderer, 1.0F, 1.0F)
        && drawAuditPose(renderer, rig, pose, background, mirrored, isolatedNode);
    if (!success) {
        error = "could not render audit pose '" + path.string() + "': "
            + std::string(SDL_GetError());
    } else if (scale == 1) {
        success = saveCurrentRender(nativeRenderer, path, error);
    } else {
        SDL_Texture* presentationTarget = SDL_CreateTexture(
            nativeRenderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            engine::Renderer2D::LogicalWidth * scale,
            engine::Renderer2D::LogicalHeight * scale);
        if (presentationTarget == nullptr
            || !SDL_SetRenderTarget(nativeRenderer, presentationTarget)
            || !clearTo(nativeRenderer, background)
            || !SDL_RenderTexture(nativeRenderer, logicalTarget, nullptr, nullptr)) {
            error = "could not present audit target at integer scale: "
                + std::string(SDL_GetError());
            success = false;
        } else {
            success = saveCurrentRender(nativeRenderer, path, error);
        }
        SDL_DestroyTexture(presentationTarget);
    }
    if (!SDL_SetRenderScale(nativeRenderer, previousScaleX, previousScaleY) && success) {
        error = "could not restore renderer scale after audit capture: "
            + std::string(SDL_GetError());
        success = false;
    }
    if (!SDL_SetRenderTarget(nativeRenderer, previousTarget) && success) {
        error = "could not restore renderer target after audit capture: "
            + std::string(SDL_GetError());
        success = false;
    }
    SDL_DestroyTexture(logicalTarget);
    return success;
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

bool RigReviewExporter::exportAuditEvidence(
    engine::Renderer2D& renderer,
    engine::Rig2D& rig,
    const engine::RigAnimation& idleAnimation,
    const engine::RigAnimation& walkAnimation,
    const std::filesystem::path& outputDirectory,
    std::string& error) const {
    error.clear();
    SDL_Renderer* nativeRenderer = renderer.native();
    if (nativeRenderer == nullptr || !renderer.initialized()) {
        error = "cannot export audit evidence without an initialized renderer";
        return false;
    }
    if (idleAnimation.keyframeCount() == 0U || walkAnimation.keyframeCount() != 8U) {
        error = "cannot export audit evidence without valid idle and eight-pose walk animations";
        return false;
    }

    for (const char* directory : {"runtime", "artifacts", "geometry", "proof", "isolated"}) {
        if (!createDirectories(outputDirectory / directory, error)) {
            return false;
        }
    }

    const engine::RigPose contactA = walkAnimation.sample(walkAnimation.keyframePhase(0));
    const engine::RigPose passingA = walkAnimation.sample(walkAnimation.keyframePhase(2));
    SDL_Texture* previousTarget = SDL_GetRenderTarget(nativeRenderer);
    if (!SDL_SetRenderTarget(nativeRenderer, nullptr)
        || !drawAuditPose(renderer, rig, contactA, {13, 11, 18, SDL_ALPHA_OPAQUE}, false)
        || !saveCurrentRender(nativeRenderer, outputDirectory / "runtime/direct_contact_a.png", error)) {
        if (error.empty()) {
            error = "could not capture direct renderer output: " + std::string(SDL_GetError());
        }
        SDL_SetRenderTarget(nativeRenderer, previousTarget);
        return false;
    }
    if (!SDL_SetRenderTarget(nativeRenderer, previousTarget)) {
        error = "could not restore renderer target after direct capture: "
            + std::string(SDL_GetError());
        return false;
    }

    const auto save = [&](const engine::RigPose& pose,
                          const std::filesystem::path& relativePath,
                          SDL_Color background,
                          bool mirrored = false,
                          int scale = 1,
                          std::string_view isolatedNode = {}) {
        return saveTargetPose(
            renderer,
            rig,
            pose,
            outputDirectory / relativePath,
            background,
            mirrored,
            scale,
            error,
            isolatedNode);
    };

    if (!save(contactA, "runtime/target_contact_a.png", {13, 11, 18, 255})
        || !save(contactA, "runtime/target_contact_a_2x.png", {13, 11, 18, 255}, false, 2)
        || !save(contactA, "artifacts/contact_a_dark.png", {13, 11, 18, 255})
        || !save(contactA, "artifacts/contact_a_neutral.png", {96, 96, 96, 255})
        || !save(contactA, "artifacts/contact_a_contrast.png", {229, 214, 171, 255})
        || !save(contactA, "geometry/contact_a_right.png", {0, 0, 0, 0})
        || !save(contactA, "geometry/contact_a_left.png", {0, 0, 0, 0}, true)
        || !save(passingA, "geometry/passing_a_right.png", {0, 0, 0, 0})
        || !save(passingA, "geometry/passing_a_left.png", {0, 0, 0, 0}, true)
        || !save(contactA, "proof/Logen_contact_a_raw.png", {0, 0, 0, 0})
        || !save(passingA, "proof/Logen_passing_a_raw.png", {0, 0, 0, 0})) {
        return false;
    }

    engine::RigPose zeroRotation = contactA;
    for (const engine::RigNode& node : rig.nodes()) {
        zeroRotation.nodes[node.id].rotationDegrees = -node.baseRotationDegrees;
    }
    if (!save(zeroRotation, "artifacts/contact_a_zero_rotation.png", {13, 11, 18, 255})) {
        return false;
    }

    for (std::size_t index = 0; index < walkAnimation.keyframeCount(); ++index) {
        std::ostringstream filename;
        filename << "runtime/motion_" << (index < 10U ? "0" : "") << index << ".png";
        if (!save(
                walkAnimation.sample(walkAnimation.keyframePhase(index)),
                filename.str(),
                {13, 11, 18, 255})) {
            return false;
        }
    }

    for (const engine::RigNode& node : rig.nodes()) {
        if (!node.texture.loaded()) {
            continue;
        }
        if (!save(
                contactA,
                std::filesystem::path("isolated") / (node.id + ".png"),
                {0, 0, 0, 0},
                false,
                1,
                node.id)) {
            return false;
        }
    }

    int outputWidth = 0;
    int outputHeight = 0;
    SDL_GetCurrentRenderOutputSize(nativeRenderer, &outputWidth, &outputHeight);
    const int version = SDL_GetVersion();
    std::ofstream metadata(outputDirectory / "runtime_metadata.json");
    if (!metadata) {
        error = "could not create runtime metadata JSON";
        return false;
    }
    metadata << "{\n"
             << "  \"sdl_version\": \"" << SDL_VERSIONNUM_MAJOR(version) << "."
             << SDL_VERSIONNUM_MINOR(version) << "." << SDL_VERSIONNUM_MICRO(version) << "\",\n"
             << "  \"renderer\": \"" << SDL_GetRendererName(nativeRenderer) << "\",\n"
             << "  \"output_size\": [" << outputWidth << ", " << outputHeight << "],\n"
             << "  \"logical_size\": [960, 540],\n"
             << "  \"target_format\": \"SDL_PIXELFORMAT_RGBA8888\",\n"
             << "  \"target_scale_mode\": \"nearest\"\n"
             << "}\n";
    if (!metadata) {
        error = "could not write runtime metadata JSON";
        return false;
    }
    rig.setMirrored(false);
    return true;
}

}
