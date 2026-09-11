#include "XYZ/Engine/Input.h"

namespace xyz::engine {

void Input::beginFrame() noexcept {
    leftMousePressed_ = false;
    rigDebugTogglePressed_ = false;
    rigReloadPressed_ = false;
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
                case SDL_SCANCODE_LEFT:
                    state_.moveLeft = pressed;
                    break;
                case SDL_SCANCODE_D:
                case SDL_SCANCODE_RIGHT:
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

bool Input::quitRequested() const noexcept {
    return quitRequested_;
}

}
