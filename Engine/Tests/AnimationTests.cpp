#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "XYZ/Engine/Animation.h"

int main() {
    const auto check = [](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "Animation test failed: " << message << "\n";
            std::exit(1);
        }
    };

    xyz::engine::Animation animation({"idle_01", "idle_02", "idle_03", "idle_04"}, 6.0F);
    check(animation.currentFrame() == 0, "animation starts at frame zero");

    animation.update(1.0F / 6.0F);
    check(animation.currentFrame() == 1, "animation advances at the requested FPS");

    animation.update(3.0F / 6.0F);
    check(animation.currentFrame() == 0, "animation loops over its frame list");

    animation.setFramesPerSecond(10.0F);
    animation.reset();
    animation.update(0.1F);
    check(animation.currentFrame() == 1, "changing FPS affects frame duration");

    std::cout << "Animation tests passed.\n";
}
