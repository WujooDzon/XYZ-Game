#include <cstdlib>
#include <iostream>

#include <SDL3/SDL.h>

#include "XYZ/Engine/Input.h"

int main() {
    const auto check = [](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "Input test failed: " << message << "\n";
            std::exit(1);
        }
    };

    xyz::engine::Input input;

    SDL_Event click{};
    click.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    click.button.button = SDL_BUTTON_LEFT;
    click.button.down = true;
    click.button.x = 412.5F;
    click.button.y = 498.0F;
    input.handleEvent(click);

    check(input.leftMousePressed(), "left mouse press is reported");
    check(input.clickPosition().x == 412.5F, "click X is retained");
    check(input.clickPosition().y == 498.0F, "click Y is retained");

    input.beginFrame();
    check(!input.leftMousePressed(), "click is one-frame state");

    SDL_Event keyDown{};
    keyDown.type = SDL_EVENT_KEY_DOWN;
    keyDown.key.down = true;
    keyDown.key.scancode = SDL_SCANCODE_A;
    input.handleEvent(keyDown);
    check(input.state().moveLeft, "A key maps to move left");

    SDL_Event keyUp = keyDown;
    keyUp.type = SDL_EVENT_KEY_UP;
    keyUp.key.down = false;
    input.handleEvent(keyUp);
    check(!input.state().moveLeft, "A key release clears move left");

    SDL_Event debugToggle{};
    debugToggle.type = SDL_EVENT_KEY_DOWN;
    debugToggle.key.down = true;
    debugToggle.key.repeat = false;
    debugToggle.key.scancode = SDL_SCANCODE_F3;
    input.handleEvent(debugToggle);
    check(input.rigDebugTogglePressed(), "F3 reports rig debug toggle");
    input.handleEvent(debugToggle);
    check(input.rigDebugTogglePressed(), "F3 repeat remains a one-frame toggle");

    input.beginFrame();
    check(!input.rigDebugTogglePressed(), "F3 toggle clears at frame start");

    debugToggle.key.repeat = true;
    input.handleEvent(debugToggle);
    check(!input.rigDebugTogglePressed(), "repeated F3 does not retrigger toggle");

    SDL_Event reload = debugToggle;
    reload.key.repeat = false;
    reload.key.scancode = SDL_SCANCODE_R;
    input.handleEvent(reload);
    check(input.rigReloadPressed(), "R reports rig reload");

    std::cout << "Input tests passed.\n";
}
