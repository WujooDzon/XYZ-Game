#include <cstdlib>
#include <iostream>
#include <string>

#include <SDL3/SDL.h>

#include "XYZ/Engine/Input.h"
#include "XYZ/Engine/Renderer2D.h"
#include "XYZ/Game/GuffmanBasementScene.h"

int main() {
    const auto check = [](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "GuffmanBasementScene test failed: " << message << "\n";
            std::exit(1);
        }
    };

    check(SDL_Init(SDL_INIT_VIDEO), "SDL video initializes");
    SDL_Window* window = SDL_CreateWindow(
        "XYZ Stage 01 Scene Test",
        xyz::engine::Renderer2D::LogicalWidth,
        xyz::engine::Renderer2D::LogicalHeight,
        SDL_WINDOW_HIDDEN);
    check(window != nullptr, "hidden SDL window is created");

    {
        xyz::engine::Renderer2D renderer(window);
        check(renderer.initialized(), "SDL renderer is created");

        xyz::game::GuffmanBasementScene scene(".");
        std::string error;
        check(scene.initialize(renderer, error), error.c_str());
        check(!scene.usesLegacyWalkFrames(), "scene uses hierarchical rig instead of legacy frames");
        check(scene.rigNodeCount() == 17, "rig definition contains pelvis and all 16 art parts");
        check(scene.walkKeyframeCount() == 8, "walk animation contains eight poses");
        check(scene.idleKeyframeCount() >= 2, "idle animation contains looping poses");
        check(scene.rigHeight() >= 182.0F && scene.rigHeight() <= 194.0F,
              "rig character height is in the intended range");
        check(scene.walkPhase() == 0.0F, "walk starts at neutral phase");
        renderer.clear();
        scene.render(renderer);
        renderer.present();

        xyz::engine::Input input;
        input.beginFrame();
        SDL_Event invalidClick{};
        invalidClick.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        invalidClick.button.button = SDL_BUTTON_LEFT;
        invalidClick.button.down = true;
        invalidClick.button.x = 700.0F;
        invalidClick.button.y = 200.0F;
        input.handleEvent(invalidClick);
        scene.update(0.1F, input, renderer);
        check(!scene.isWalking(), "click outside floor band does not move");
        check(scene.playerX() == xyz::game::GuffmanBasementScene::PlayerInitialX, "invalid click preserves X");

        input.beginFrame();
        SDL_Event validClick{};
        validClick.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        validClick.button.button = SDL_BUTTON_LEFT;
        validClick.button.down = true;
        validClick.button.x = 700.0F;
        validClick.button.y = 480.0F;
        input.handleEvent(validClick);
        scene.update(0.1F, input, renderer);
        check(scene.isWalking(), "floor click starts target movement");
        check(scene.playerX() > xyz::game::GuffmanBasementScene::PlayerInitialX, "floor click moves right");
        check(scene.playerFacing() == xyz::game::Facing::Right, "right target faces right");
        check(scene.walkPhase() > 0.0F, "walk phase advances from displacement");

        input.beginFrame();
        for (int frame = 0; frame < 120; ++frame) {
            scene.update(1.0F / 60.0F, input, renderer);
        }
        check(!scene.isWalking(), "arrival switches back to idle");
        check(scene.playerX() == 700.0F, "arrival reaches clicked X");
        check(scene.walkPhase() == 0.0F, "stopping returns walk phase to neutral");

        input.beginFrame();
        SDL_Event leftClick{};
        leftClick.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        leftClick.button.button = SDL_BUTTON_LEFT;
        leftClick.button.down = true;
        leftClick.button.x = 100.0F;
        leftClick.button.y = 480.0F;
        input.handleEvent(leftClick);
        scene.update(0.1F, input, renderer);
        check(scene.isWalking(), "second floor click starts another walk");
        check(scene.playerFacing() == xyz::game::Facing::Left, "left target faces left");

        input.beginFrame();
        SDL_Event keyDown{};
        keyDown.type = SDL_EVENT_KEY_DOWN;
        keyDown.key.down = true;
        keyDown.key.scancode = SDL_SCANCODE_D;
        input.handleEvent(keyDown);
        scene.update(0.2F, input, renderer);
        check(scene.isWalking(), "D key starts manual movement");
        check(scene.playerFacing() == xyz::game::Facing::Right, "D key faces right");

        input.beginFrame();
        SDL_Event keyUp = keyDown;
        keyUp.type = SDL_EVENT_KEY_UP;
        keyUp.key.down = false;
        input.handleEvent(keyUp);
        scene.update(0.1F, input, renderer);
        check(scene.isWalking(), "releasing D enters deceleration");
        for (int frame = 0; frame < 30; ++frame) {
            input.beginFrame();
            scene.update(1.0F / 60.0F, input, renderer);
        }
        check(!scene.isWalking(), "deceleration returns to idle");

        input.beginFrame();
        SDL_Event debugToggle{};
        debugToggle.type = SDL_EVENT_KEY_DOWN;
        debugToggle.key.down = true;
        debugToggle.key.repeat = false;
        debugToggle.key.scancode = SDL_SCANCODE_F3;
        input.handleEvent(debugToggle);
        scene.update(0.0F, input, renderer);
        check(scene.rigDebugEnabled(), "F3 enables rig debug overlay");
        renderer.clear();
        scene.render(renderer);
        renderer.present();

        input.beginFrame();
        SDL_Event reload = debugToggle;
        reload.key.scancode = SDL_SCANCODE_R;
        input.handleEvent(reload);
        scene.update(0.0F, input, renderer);
        check(scene.rigReloadError().empty(), "R reloads valid rig data");
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "GuffmanBasementScene tests passed.\n";
}
