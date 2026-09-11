#pragma once

#include <SDL3/SDL.h>

#include "XYZ/Engine/InputState.h"

namespace xyz::engine {

class Input {
public:
    void beginFrame() noexcept;
    void handleEvent(const SDL_Event& event) noexcept;

    [[nodiscard]] InputState state() const noexcept;
    [[nodiscard]] SDL_FPoint mousePosition() const noexcept;
    [[nodiscard]] SDL_FPoint clickPosition() const noexcept;
    [[nodiscard]] bool leftMousePressed() const noexcept;
    [[nodiscard]] bool rigDebugTogglePressed() const noexcept;
    [[nodiscard]] bool rigReloadPressed() const noexcept;
    [[nodiscard]] bool quitRequested() const noexcept;

private:
    InputState state_{};
    SDL_FPoint mousePosition_{0.0F, 0.0F};
    SDL_FPoint clickPosition_{0.0F, 0.0F};
    bool leftMousePressed_ = false;
    bool rigDebugTogglePressed_ = false;
    bool rigReloadPressed_ = false;
    bool quitRequested_ = false;
};

}
