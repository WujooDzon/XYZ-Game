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

} // namespace

int main() {
    const std::filesystem::path root = std::filesystem::current_path();
    const std::filesystem::path rigDirectory =
        root / "Assets" / "Characters" / "Logen" / "RigV3";
    const std::filesystem::path outputDirectory =
        std::filesystem::temp_directory_path() / "xyz-stage01d-rig-review-test";
    std::error_code filesystemError;
    std::filesystem::remove_all(outputDirectory, filesystemError);

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
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    std::filesystem::remove_all(outputDirectory, filesystemError);
    std::cout << "RigReviewExporter tests passed.\n";
}
