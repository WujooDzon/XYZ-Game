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
    const auto first = controller.update(100.0F, 112.0F, 0.10F, true, 1.0F / 60.0F);
    const auto second = controller.update(106.0F, 112.0F, 0.12F, true, 1.0F / 60.0F);
    check(first.support != xyz::game::FootPlantController::SupportFoot::None,
          "walking selects a support foot");
    check(std::fabs(second.rootCorrectionX) > 0.0F,
          "visual planting produces a render-only correction");
    check(controller.update(106.0F, 112.0F, 0.12F, false, 1.0F / 60.0F).rootCorrectionX == 0.0F,
          "idle clears visual correction");

    std::cout << "FootPlantController tests passed.\n";
}
