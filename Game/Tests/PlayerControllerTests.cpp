#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>

#include "XYZ/Game/PlayerController.h"

int main() {
    const auto check = [](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "PlayerController test failed: " << message << "\n";
            std::exit(1);
        }
    };

    xyz::game::PlayerController player({80.0F, 880.0F}, 335.0F, 455.0F, 220.0F);
    player.setMoveTarget(700.0F);
    player.update(0.05F, {});
    check(player.isMoving(), "target movement reports walking");
    check(player.x() > 335.0F && player.x() < 346.0F, "target movement eases into motion");
    check(player.velocity() > 0.0F && player.velocity() < 220.0F, "target movement accelerates");
    check(player.facing() == xyz::game::Facing::Right, "right target faces right");

    const float xAfterAcceleration = player.x();
    player.update(0.5F, {});
    check(player.x() > xAfterAcceleration, "target continues after acceleration");
    check(player.velocity() > 0.0F, "target reaches cruising velocity");

    for (int frame = 0; frame < 300; ++frame) {
        player.update(1.0F / 60.0F, {});
    }
    check(!player.isMoving(), "arrival reports idle");
    check(std::fabs(player.x() - 700.0F) < 0.001F, "arrival lands on target");
    check(std::fabs(player.velocity()) < 0.001F, "arrival stops velocity");

    player.setMoveTarget(-100.0F);
    player.update(0.1F, {});
    check(player.facing() == xyz::game::Facing::Left, "left target faces left");
    for (int frame = 0; frame < 300; ++frame) {
        player.update(1.0F / 60.0F, {});
    }
    check(player.x() == 80.0F, "target is clamped to left bound");

    xyz::engine::InputState right{};
    right.moveRight = true;
    player.setMoveTarget(400.0F);
    player.update(0.1F, right);
    check(player.targetX() == std::nullopt, "manual input cancels target movement");
    check(player.facing() == xyz::game::Facing::Right, "manual right input faces right");
    check(player.velocity() > 0.0F, "manual input accelerates");

    player.update(0.05F, {});
    check(player.isMoving(), "manual movement coasts during deceleration");
    for (int frame = 0; frame < 60; ++frame) {
        player.update(1.0F / 60.0F, {});
    }
    check(!player.isMoving(), "deceleration returns to idle");
    check(std::fabs(player.velocity()) < 0.001F, "deceleration reaches zero velocity");

    std::cout << "PlayerController tests passed.\n";
}
