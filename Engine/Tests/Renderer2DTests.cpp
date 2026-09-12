#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "XYZ/Engine/Renderer2D.h"
#include "XYZ/Engine/Texture.h"

int main() {
    const auto check = [](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "Renderer2D test failed: " << message << "\n";
            std::exit(1);
        }
    };

    check(SDL_Init(SDL_INIT_VIDEO), "SDL video initializes");
    SDL_Window* window = SDL_CreateWindow(
        "XYZ Renderer Test",
        xyz::engine::Renderer2D::LogicalWidth,
        xyz::engine::Renderer2D::LogicalHeight,
        SDL_WINDOW_HIDDEN);
    check(window != nullptr, "hidden window is created");

    {
        xyz::engine::Renderer2D renderer(window);
        check(renderer.initialized(), "renderer initializes");

        const std::filesystem::path testDirectory =
            std::filesystem::temp_directory_path() / "xyz-logen-visible-texture-tests";
        std::filesystem::create_directories(testDirectory);
        const auto writeFixture = [&](const std::filesystem::path& path, bool visible) {
            SDL_Surface* surface = SDL_CreateSurface(10, 12, SDL_PIXELFORMAT_RGBA32);
            check(surface != nullptr, "texture fixture surface is created");
            check(SDL_ClearSurface(surface, 0.0F, 0.0F, 0.0F, 0.0F),
                  "texture fixture starts transparent");
            if (visible) {
                const SDL_Rect rectangle{3, 2, 4, 7};
                check(SDL_FillSurfaceRect(
                          surface,
                          &rectangle,
                          SDL_MapSurfaceRGBA(surface, 80, 60, 40, 255)),
                      "visible fixture rectangle is filled");
            }
            check(IMG_SavePNG(surface, path.string().c_str()), "texture fixture PNG is saved");
            SDL_DestroySurface(surface);
        };
        const auto paddedPath = testDirectory / "padded.png";
        const auto emptyPath = testDirectory / "empty.png";
        writeFixture(paddedPath, true);
        writeFixture(emptyPath, false);

        xyz::engine::Texture padded;
        check(padded.load(renderer.native(), paddedPath, "padded texture"),
              "padded texture loads");
        const auto visibleBounds = padded.visibleBounds();
        check(visibleBounds.has_value(), "padded texture exposes visible alpha bounds");
        check(visibleBounds->x == 3 && visibleBounds->y == 2
                  && visibleBounds->w == 4 && visibleBounds->h == 7,
              "visible alpha bounds ignore transparent file padding");

        xyz::engine::Texture empty;
        check(!empty.load(renderer.native(), emptyPath, "empty texture"),
              "fully transparent texture fails with a readable load error");

        xyz::engine::Texture texture;
        check(texture.load(
                  renderer.native(),
                  "Assets/Locations/GuffmanBasement/GuffmanBasement_BG_v1.png",
                  "renderer test texture"),
              "test texture loads");
        check(renderer.drawTexture(
                  texture,
                  {20.0F, 20.0F, 40.0F, 40.0F},
                  15.0F,
                  {8.0F, 8.0F}),
              "pivot-aware texture draws");
        check(renderer.drawTexture(
                  texture,
                  {25.0F, 25.0F, 40.0F, 40.0F},
                  SDL_Color{255, 255, 255, 96}),
              "modulated texture draws");
        check(renderer.drawDebugLine(
                  {10.0F, 10.0F},
                  {20.0F, 20.0F},
                  {255, 0, 0, SDL_ALPHA_OPAQUE}),
              "debug line draws");
        check(renderer.drawDebugRect(
                  {10.0F, 10.0F, 20.0F, 20.0F},
                  {0, 255, 0, SDL_ALPHA_OPAQUE}),
              "debug rect draws");

        std::filesystem::remove_all(testDirectory);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "Renderer2D tests passed.\n";
}
