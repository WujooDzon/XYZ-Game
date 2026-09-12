#pragma once

#include <SDL3/SDL.h>

#include "XYZ/Engine/InputState.h"

namespace xyz::engine {

struct RigCalibrationInput {
    bool nextNode = false;
    bool previousNode = false;
    bool stepPrevious = false;
    bool stepNext = false;
    bool save = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool moveUp = false;
    bool moveDown = false;
    bool rotateLeft = false;
    bool rotateRight = false;
    bool pivotLeft = false;
    bool pivotRight = false;
    bool pivotUp = false;
    bool pivotDown = false;
    bool largeStep = false;
};

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
    [[nodiscard]] bool masterReferenceTogglePressed() const noexcept;
    [[nodiscard]] bool rigPauseTogglePressed() const noexcept;
    [[nodiscard]] bool footPlantTogglePressed() const noexcept;
    [[nodiscard]] bool rigReviewCapturePressed() const noexcept;
    [[nodiscard]] const RigCalibrationInput& rigCalibration() const noexcept;
    [[nodiscard]] bool quitRequested() const noexcept;

private:
    InputState state_{};
    SDL_FPoint mousePosition_{0.0F, 0.0F};
    SDL_FPoint clickPosition_{0.0F, 0.0F};
    bool leftMousePressed_ = false;
    bool rigDebugTogglePressed_ = false;
    bool rigReloadPressed_ = false;
    bool masterReferenceTogglePressed_ = false;
    bool rigPauseTogglePressed_ = false;
    bool footPlantTogglePressed_ = false;
    bool rigReviewCapturePressed_ = false;
    RigCalibrationInput rigCalibration_{};
    bool quitRequested_ = false;
};

}
