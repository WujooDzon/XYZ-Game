#include "XYZ/Engine/Application.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <utility>

#include <SDL3/SDL.h>

#include "XYZ/Engine/Input.h"
#include "XYZ/Engine/Renderer2D.h"
#include "XYZ/Engine/Scene.h"

namespace xyz::engine {

Application::Application(std::string title)
    : title_(std::move(title)) {}

int Application::run(Scene& scene) const {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Could not initialize SDL3: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        title_.c_str(),
        Renderer2D::LogicalWidth,
        Renderer2D::LogicalHeight,
        SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        std::cerr << "Could not create game window: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_SetWindowAspectRatio(window, 16.0F / 9.0F, 16.0F / 9.0F);

    int result = 0;
    {
        Renderer2D renderer(window);
        if (!renderer.initialized()) {
            result = 1;
        } else {
            std::string error;
            if (!scene.initialize(renderer, error)) {
                std::cerr << "Could not initialize game scene: " << error << "\n";
                result = 1;
            } else {
                Input input;
                const Uint64 performanceFrequency = SDL_GetPerformanceFrequency();
                Uint64 previousCounter = SDL_GetPerformanceCounter();

                while (!input.quitRequested()) {
                    input.beginFrame();
                    SDL_Event event;
                    while (SDL_PollEvent(&event)) {
                        input.handleEvent(event);
                    }

                    const Uint64 currentCounter = SDL_GetPerformanceCounter();
                    const double elapsed = static_cast<double>(currentCounter - previousCounter)
                        / static_cast<double>(performanceFrequency);
                    previousCounter = currentCounter;
                    const float deltaSeconds = std::min(0.1F, static_cast<float>(elapsed));

                    scene.update(deltaSeconds, input, renderer);
                    renderer.clear();
                    scene.render(renderer);
                    renderer.present();
                }
            }
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}

}
