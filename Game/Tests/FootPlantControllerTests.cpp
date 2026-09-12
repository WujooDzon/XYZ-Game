#include <cmath>
#include <cstdlib>
#include <iostream>

#include "XYZ/Game/FootPlantController.h"

int main() {
    const auto check = [](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FootPlantController test failed: " << message << "\n";
            std::exit(1);
        }
    };

    xyz::game::FootPlantController controller;
    const auto farSupport = controller.update(100.0F, 112.0F, 0.10F, true, 1.0F / 60.0F);
    check(static_cast<int>(farSupport.support) == 2,
          "phase 0.10 selects the far support leg");
    const auto transfer = controller.update(106.0F, 112.0F, 0.40F, true, 1.0F / 60.0F);
    check(static_cast<int>(transfer.support) == 0,
          "phase 0.40 enters the transfer window");
    const auto nearSupport = controller.update(106.0F, 112.0F, 0.60F, true, 1.0F / 60.0F);
    check(static_cast<int>(nearSupport.support) == 1,
          "phase 0.60 selects the near support leg");
    const auto nearCorrection = controller.update(112.0F, 112.0F, 0.62F, true, 1.0F / 60.0F);
    check(std::fabs(nearCorrection.rootCorrectionX) > 0.0F,
          "visual planting produces a render-only correction");
    check(controller.update(106.0F, 112.0F, 0.60F, false, 1.0F / 60.0F).rootCorrectionX == 0.0F,
          "idle clears visual correction");

    controller.reset();
    controller.update(100.0F, 160.0F, 0.10F, true, 0.08F);
    controller.update(104.0F, 164.0F, 0.40F, true, 0.08F);
    const auto nearEntry = controller.update(140.0F, 168.0F, 0.50F, true, 0.0F);
    check(std::fabs(nearEntry.plantedWorldX - 140.0F) < 0.001F,
          "near support captures the current near foot world position");
    check(std::fabs(nearEntry.desiredCorrectionX) < 0.001F,
          "new support starts with its own contact already planted");
    const auto nearMoved = controller.update(145.0F, 168.0F, 0.55F, true, 0.08F);
    check(std::fabs(nearMoved.rootCorrectionX + 5.0F) < 0.001F,
          "near support locks the captured near anchor");

    controller.update(150.0F, 170.0F, 0.85F, true, 0.08F);
    controller.update(154.0F, 174.0F, 0.90F, true, 0.08F);
    const auto farEntry = controller.update(210.0F, 195.0F, 0.0F, true, 0.0F);
    check(std::fabs(farEntry.plantedWorldX - 195.0F) < 0.001F,
          "far support captures the current far foot world position");

    std::cout << "FootPlantController tests passed.\n";
}
