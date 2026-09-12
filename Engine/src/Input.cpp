#include "XYZ/Engine/Input.h"

namespace xyz::engine {

void Input::beginFrame() noexcept {
    leftMousePressed_ = false;
    rigDebugTogglePressed_ = false;
    rigReloadPressed_ = false;
    masterReferenceTogglePressed_ = false;
    rigPauseTogglePressed_ = false;
    footPlantTogglePressed_ = false;
    rigReviewCapturePressed_ = false;
    rigCalibration_ = {};
}

void Input::handleEvent(const SDL_Event& event) noexcept {
    switch (event.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            quitRequested_ = true;
            break;

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            const bool pressed = event.key.down;
            switch (event.key.scancode) {
                case SDL_SCANCODE_A:
                    state_.moveLeft = pressed;
                    break;
                case SDL_SCANCODE_D:
                    state_.moveRight = pressed;
                    break;
                case SDL_SCANCODE_ESCAPE:
                    if (pressed) {
                        quitRequested_ = true;
                    }
                    break;
                case SDL_SCANCODE_F3:
                    if (pressed && !event.key.repeat) {
                        rigDebugTogglePressed_ = true;
                    }
                    break;
                case SDL_SCANCODE_R:
                    if (pressed && !event.key.repeat) {
                        rigReloadPressed_ = true;
                    }
                    break;
                case SDL_SCANCODE_F4:
                    if (pressed && !event.key.repeat) {
                        masterReferenceTogglePressed_ = true;
                    }
                    break;
                case SDL_SCANCODE_F5:
                    if (pressed && !event.key.repeat) {
                        rigPauseTogglePressed_ = true;
                    }
                    break;
                case SDL_SCANCODE_F6:
                    if (pressed && !event.key.repeat) {
                        footPlantTogglePressed_ = true;
                    }
                    break;
                case SDL_SCANCODE_F7:
                    if (pressed && !event.key.repeat) {
                        rigReviewCapturePressed_ = true;
                    }
                    break;
                case SDL_SCANCODE_TAB:
                    if (pressed && !event.key.repeat) {
                        if ((event.key.mod & SDL_KMOD_SHIFT) != 0) {
                            rigCalibration_.previousNode = true;
                        } else {
                            rigCalibration_.nextNode = true;
                        }
                    }
                    break;
                case SDL_SCANCODE_COMMA:
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.stepPrevious = true;
                    }
                    break;
                case SDL_SCANCODE_PERIOD:
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.stepNext = true;
                    }
                    break;
                case SDL_SCANCODE_S:
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.save = true;
                    }
                    break;
                case SDL_SCANCODE_Q:
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.rotateLeft = true;
                        rigCalibration_.largeStep = (event.key.mod & SDL_KMOD_SHIFT) != 0;
                    }
                    break;
                case SDL_SCANCODE_E:
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.rotateRight = true;
                        rigCalibration_.largeStep = (event.key.mod & SDL_KMOD_SHIFT) != 0;
                    }
                    break;
                case SDL_SCANCODE_J:
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.pivotLeft = true;
                    }
                    break;
                case SDL_SCANCODE_L:
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.pivotRight = true;
                    }
                    break;
                case SDL_SCANCODE_I:
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.pivotUp = true;
                    }
                    break;
                case SDL_SCANCODE_K:
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.pivotDown = true;
                    }
                    break;
                case SDL_SCANCODE_UP:
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.moveUp = true;
                        rigCalibration_.largeStep = (event.key.mod & SDL_KMOD_SHIFT) != 0;
                    }
                    break;
                case SDL_SCANCODE_DOWN:
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.moveDown = true;
                        rigCalibration_.largeStep = (event.key.mod & SDL_KMOD_SHIFT) != 0;
                    }
                    break;
                case SDL_SCANCODE_LEFT:
                    state_.moveLeft = pressed;
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.moveLeft = true;
                        rigCalibration_.largeStep = (event.key.mod & SDL_KMOD_SHIFT) != 0;
                    }
                    break;
                case SDL_SCANCODE_RIGHT:
                    state_.moveRight = pressed;
                    if (pressed && !event.key.repeat) {
                        rigCalibration_.moveRight = true;
                        rigCalibration_.largeStep = (event.key.mod & SDL_KMOD_SHIFT) != 0;
                    }
                    break;
                default:
                    break;
            }
            break;
        }

        case SDL_EVENT_MOUSE_MOTION:
            mousePosition_ = {event.motion.x, event.motion.y};
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            mousePosition_ = {event.button.x, event.button.y};
            if (event.button.button == SDL_BUTTON_LEFT && event.button.down) {
                clickPosition_ = mousePosition_;
                leftMousePressed_ = true;
            }
            break;

        default:
            break;
    }
}

InputState Input::state() const noexcept {
    return state_;
}

SDL_FPoint Input::mousePosition() const noexcept {
    return mousePosition_;
}

SDL_FPoint Input::clickPosition() const noexcept {
    return clickPosition_;
}

bool Input::leftMousePressed() const noexcept {
    return leftMousePressed_;
}

bool Input::rigDebugTogglePressed() const noexcept {
    return rigDebugTogglePressed_;
}

bool Input::rigReloadPressed() const noexcept {
    return rigReloadPressed_;
}

bool Input::masterReferenceTogglePressed() const noexcept {
    return masterReferenceTogglePressed_;
}

bool Input::rigPauseTogglePressed() const noexcept {
    return rigPauseTogglePressed_;
}

bool Input::footPlantTogglePressed() const noexcept {
    return footPlantTogglePressed_;
}

bool Input::rigReviewCapturePressed() const noexcept {
    return rigReviewCapturePressed_;
}

const RigCalibrationInput& Input::rigCalibration() const noexcept {
    return rigCalibration_;
}

bool Input::quitRequested() const noexcept {
    return quitRequested_;
}

}
