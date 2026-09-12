#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "XYZ/Engine/Rig2D.h"
#include "XYZ/Engine/RigAnimation.h"
#include "XYZ/Engine/Renderer2D.h"
#include "XYZ/Game/RigReviewExporter.h"

namespace {

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "RigReviewExporter test failed: " << message << "\n";
        std::exit(1);
    }
}

bool nearestTwoXMatches(SDL_Surface* source, SDL_Surface* scaled) {
    SDL_Surface* sourceRgba = SDL_ConvertSurface(source, SDL_PIXELFORMAT_RGBA32);
    SDL_Surface* scaledRgba = SDL_ConvertSurface(scaled, SDL_PIXELFORMAT_RGBA32);
    check(sourceRgba != nullptr && scaledRgba != nullptr,
          "renderer comparison surfaces convert to RGBA32");
    bool matches = scaledRgba->w == sourceRgba->w * 2
        && scaledRgba->h == sourceRgba->h * 2;
    for (int y = 0; matches && y < scaledRgba->h; ++y) {
        const auto* scaledRow = reinterpret_cast<const Uint32*>(
            static_cast<const Uint8*>(scaledRgba->pixels)
            + static_cast<std::size_t>(y) * static_cast<std::size_t>(scaledRgba->pitch));
        const auto* sourceRow = reinterpret_cast<const Uint32*>(
            static_cast<const Uint8*>(sourceRgba->pixels)
            + static_cast<std::size_t>(y / 2) * static_cast<std::size_t>(sourceRgba->pitch));
        for (int x = 0; x < scaledRgba->w; ++x) {
            if (scaledRow[x] != sourceRow[x / 2]) {
                matches = false;
                break;
            }
        }
    }
    SDL_DestroySurface(sourceRgba);
    SDL_DestroySurface(scaledRgba);
    return matches;
}

} // namespace

int main() {
    const std::filesystem::path root = std::filesystem::current_path();
    const std::filesystem::path rigDirectory =
        root / "Assets" / "Characters" / "Logen" / "RigV3";
    const char* configuredOutput = std::getenv("XYZ_LOGEN_AUDIT_OUTPUT");
    const bool preserveOutput = configuredOutput != nullptr && configuredOutput[0] != '\0';
    const std::filesystem::path outputDirectory = preserveOutput
        ? std::filesystem::path(configuredOutput)
        : std::filesystem::temp_directory_path() / "xyz-stage01d-rig-review-test";
    std::error_code filesystemError;
    if (!preserveOutput) {
        std::filesystem::remove_all(outputDirectory, filesystemError);
    }

    check(SDL_Init(SDL_INIT_VIDEO), "SDL video initializes");
    SDL_Window* window = SDL_CreateWindow(
        "XYZ Rig Review Exporter Test",
        xyz::engine::Renderer2D::LogicalWidth,
        xyz::engine::Renderer2D::LogicalHeight,
        SDL_WINDOW_HIDDEN);
    check(window != nullptr, "hidden SDL window is created");

    {
        xyz::engine::Renderer2D renderer(window);
        check(renderer.initialized(), "SDL renderer is created");

        xyz::engine::Rig2D rig;
        xyz::engine::RigAnimation idleAnimation;
        xyz::engine::RigAnimation walkAnimation;
        std::string error;
        check(rig.loadDefinition(renderer, rigDirectory / "Logen_rig_v3_definition.json", error), error);
        check(idleAnimation.load(rigDirectory / "Logen_idle_v3.json", error), error);
        check(walkAnimation.load(rigDirectory / "Logen_walk_v3.json", error), error);

        xyz::game::RigReviewExporter exporter;
        check(exporter.exportWalkReview(
                  renderer,
                  rig,
                  idleAnimation,
                  walkAnimation,
                  outputDirectory,
                  error),
              error);

        const std::filesystem::path auditDirectory = outputDirectory / "audit";
        check(exporter.exportAuditEvidence(
                  renderer,
                  rig,
                  idleAnimation,
                  walkAnimation,
                  auditDirectory,
                  error),
              error);

        const std::array<std::string, 10> expectedFiles{
            "00_idle.png",
            "01_contact_a.png",
            "02_down_a.png",
            "03_passing_a.png",
            "04_up_a.png",
            "05_contact_b.png",
            "06_down_b.png",
            "07_passing_b.png",
            "08_up_b.png",
            "Logen_walk_contact_sheet.png"};
        for (const std::string& filename : expectedFiles) {
            const std::filesystem::path path = outputDirectory / filename;
            check(std::filesystem::is_regular_file(path), "export exists: " + filename);
            SDL_Surface* image = IMG_Load(path.string().c_str());
            check(image != nullptr, "export is a readable PNG: " + filename);
            check(image->w == xyz::engine::Renderer2D::LogicalWidth
                      && image->h == xyz::engine::Renderer2D::LogicalHeight,
                  "export is exactly 960x540: " + filename);
            SDL_DestroySurface(image);
        }


        const std::array<std::string, 14> expectedAuditPngs{
            "runtime/direct_contact_a.png",
            "runtime/target_contact_a.png",
            "runtime/target_contact_a_2x.png",
            "runtime/motion_00.png",
            "runtime/motion_02.png",
            "runtime/motion_04.png",
            "runtime/motion_06.png",
            "artifacts/contact_a_dark.png",
            "artifacts/contact_a_neutral.png",
            "artifacts/contact_a_contrast.png",
            "artifacts/contact_a_zero_rotation.png",
            "geometry/contact_a_right.png",
            "geometry/contact_a_left.png",
            "proof/Logen_contact_a_raw.png"};
        for (const std::string& relativePath : expectedAuditPngs) {
            const std::filesystem::path path = auditDirectory / relativePath;
            check(std::filesystem::is_regular_file(path), "audit export exists: " + relativePath);
            SDL_Surface* image = IMG_Load(path.string().c_str());
            check(image != nullptr, "audit export is a readable PNG: " + relativePath);
            const bool isTwoX = relativePath == "runtime/target_contact_a_2x.png";
            check(image->w == xyz::engine::Renderer2D::LogicalWidth * (isTwoX ? 2 : 1)
                      && image->h == xyz::engine::Renderer2D::LogicalHeight * (isTwoX ? 2 : 1),
                  "audit export has expected dimensions: " + relativePath);
            SDL_DestroySurface(image);
        }
        check(std::filesystem::is_regular_file(auditDirectory / "runtime_metadata.json"),
              "runtime metadata is exported");
        SDL_Surface* targetOneX = IMG_Load(
            (auditDirectory / "runtime/target_contact_a.png").string().c_str());
        SDL_Surface* targetTwoX = IMG_Load(
            (auditDirectory / "runtime/target_contact_a_2x.png").string().c_str());
        check(targetOneX != nullptr && targetTwoX != nullptr,
              "renderer comparison targets load");
        check(nearestTwoXMatches(targetOneX, targetTwoX),
              "2x target is the exact nearest-neighbor presentation of the 960x540 target");
        SDL_DestroySurface(targetOneX);
        SDL_DestroySurface(targetTwoX);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    if (!preserveOutput) {
        std::filesystem::remove_all(outputDirectory, filesystemError);
    }
    std::cout << "RigReviewExporter tests passed.\n";
}
