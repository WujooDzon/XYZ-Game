#include <cmath>
#include <cstdlib>
#include <filesystem>
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
        check(scene.rigNodeCount() == 11, "V3 rig contains pelvis and exactly 10 active art parts");
        check(scene.walkKeyframeCount() == 8, "walk animation contains eight poses");
        check(scene.idleKeyframeCount() >= 2, "idle animation contains looping poses");
        check(scene.rigHeight() >= 182.0F && scene.rigHeight() <= 194.0F,
              "rig character height is in the intended range");
        check(scene.walkPhase() == 0.0F, "walk starts at neutral phase");
        check(scene.footPlantCorrectionEnabled(), "foot planting defaults to enabled");
        const SDL_FRect masterRight = scene.masterReferenceDestination();
        constexpr float masterVisibleLeft = 261.0F;
        constexpr float masterVisibleTop = 149.0F;
        constexpr float masterVisibleWidth = 524.0F;
        constexpr float masterVisibleHeight = 1169.0F;
        const float masterScale =
            xyz::game::GuffmanBasementScene::PlayerVisibleHeight / masterVisibleHeight;
        check(std::fabs(
                  masterRight.x + (masterVisibleLeft + masterVisibleWidth * 0.5F) * masterScale
                  - xyz::game::GuffmanBasementScene::PlayerInitialX) < 0.1F,
              "master visible art is centered on the player root");
        check(std::fabs(
                  masterRight.y + (masterVisibleTop + masterVisibleHeight) * masterScale
                  - xyz::game::GuffmanBasementScene::PlayerBaselineY) < 0.1F,
              "master visible sole line shares the gameplay baseline");
        check(std::fabs(masterVisibleHeight * masterScale
                        - xyz::game::GuffmanBasementScene::PlayerVisibleHeight) < 0.1F,
              "master visible content, not file padding, is scaled to 188 pixels");
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

        input.beginFrame();
        SDL_Event masterToggle = debugToggle;
        masterToggle.key.scancode = SDL_SCANCODE_F4;
        masterToggle.key.repeat = false;
        input.handleEvent(masterToggle);
        scene.update(0.0F, input, renderer);
        check(scene.masterReferenceEnabled(), "F4 enables the master reference overlay");

        input.beginFrame();
        SDL_Event pauseToggle = debugToggle;
        pauseToggle.key.scancode = SDL_SCANCODE_F5;
        pauseToggle.key.repeat = false;
        input.handleEvent(pauseToggle);
        scene.update(0.0F, input, renderer);
        check(scene.rigPaused(), "F5 pauses rig animation");

        input.beginFrame();
        SDL_Event footPlantToggle = debugToggle;
        footPlantToggle.key.scancode = SDL_SCANCODE_F6;
        footPlantToggle.key.repeat = false;
        input.handleEvent(footPlantToggle);
        scene.update(0.0F, input, renderer);
        check(!scene.footPlantCorrectionEnabled(), "F6 disables foot planting");
        check(scene.visualRootCorrectionX() == 0.0F, "disabled foot planting clears correction");

        input.beginFrame();
        input.handleEvent(footPlantToggle);
        scene.update(0.0F, input, renderer);
        check(scene.footPlantCorrectionEnabled(), "F6 re-enables foot planting");

        input.beginFrame();
        SDL_Event rigReviewCapture = debugToggle;
        rigReviewCapture.key.scancode = SDL_SCANCODE_F7;
        rigReviewCapture.key.repeat = false;
        input.handleEvent(rigReviewCapture);
        scene.update(0.0F, input, renderer);
        check(scene.rigReviewError().empty(), "F7 exports the V3 rig review without errors");
        check(std::filesystem::is_regular_file(
                  "Build/RigReview/Logen_walk_contact_sheet.png"),
              "F7 writes the walk contact sheet");

        input.beginFrame();
        SDL_Event stepPose = debugToggle;
        stepPose.key.scancode = SDL_SCANCODE_PERIOD;
        stepPose.key.repeat = false;
        input.handleEvent(stepPose);
        scene.update(0.0F, input, renderer);
        check(scene.rigKeyframeLabel() == "down_a", "period steps to the next walk pose");

        input.beginFrame();
        SDL_Event selectNext = debugToggle;
        selectNext.key.scancode = SDL_SCANCODE_TAB;
        selectNext.key.repeat = false;
        selectNext.key.mod = SDL_KMOD_NONE;
        input.handleEvent(selectNext);
        scene.update(0.0F, input, renderer);
        check(scene.selectedRigNodeIndex() == 1U, "TAB selects the next rig node");

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
