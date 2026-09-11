#include <cstdlib>
#include <iostream>
#include <string>

#include <SDL3/SDL.h>

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
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "Renderer2D tests passed.\n";
}
