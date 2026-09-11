#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "XYZ/Engine/RigAnimation.h"

namespace {

std::filesystem::path writeFile(
    const std::filesystem::path& directory,
    const std::string& name,
    const std::string& contents) {
    const std::filesystem::path path = directory / name;
    std::ofstream file(path);
    file << contents;
    return path;
}

} // namespace

int main() {
    const auto check = [](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "RigAnimation test failed: " << message << "\n";
            std::exit(1);
        }
    };

    const std::filesystem::path testDirectory =
        std::filesystem::temp_directory_path() / "xyz-stage01b-rig-animation-tests";
    std::filesystem::create_directories(testDirectory);

    std::string walkJson =
        R"({
  "name": "walk",
  "loop": true,
  "stride_distance": 64.0,
  "duration_seconds": 1.0,
  "keyframes": [)";
    for (int index = 0; index < 8; ++index) {
        if (index != 0) {
            walkJson += ",";
        }
        const float leftRotation = -18.0F + static_cast<float>(index) * 5.0F;
        const float rightRotation = 18.0F - static_cast<float>(index) * 5.0F;
        walkJson +=
            "{\"phase\":" + std::to_string(static_cast<float>(index) / 8.0F)
            + ",\"nodes\":{\"left_thigh\":{\"position\":[0,0],\"rotation\":"
            + std::to_string(leftRotation)
            + ",\"scale\":[1,1]},\"right_thigh\":{\"position\":[0,0],\"rotation\":"
            + std::to_string(rightRotation)
            + ",\"scale\":[1,1]}}}";
    }
    walkJson += "]}";

    const auto walkPath = writeFile(testDirectory, "walk.json", walkJson);
    std::string error;
    xyz::engine::RigAnimation walk;
    check(walk.load(walkPath, error), error.c_str());
    check(walk.keyframeCount() == 8, "walk has eight keyframes");
    const auto halfway = walk.sample(0.0625F);
    check(std::fabs(halfway.nodes.at("left_thigh").rotationDegrees + 15.5F) < 0.001F,
          "poses interpolate");

    xyz::engine::RigAnimator animator;
    animator.setAnimation(&walk);
    animator.advanceByDistance(walk.strideDistance());
    check(std::fabs(animator.phase()) < 0.001F, "one stride loops phase");
    animator.advanceByDistance(walk.strideDistance() * 0.5F);
    check(std::fabs(animator.phase() - 0.5F) < 0.001F, "walk phase uses distance");

    const auto idlePath = writeFile(
        testDirectory,
        "idle.json",
        R"({
  "name": "idle",
  "loop": true,
  "stride_distance": 1.0,
  "duration_seconds": 2.4,
  "keyframes": [
    {"phase":0.0,"nodes":{"torso":{"position":[0,0],"rotation":0,"scale":[1,1]}}},
    {"phase":0.5,"nodes":{"torso":{"position":[0,-1],"rotation":1,"scale":[1,1]}}}
  ]
})");
    xyz::engine::RigAnimation idle;
    check(idle.load(idlePath, error), error.c_str());
    animator.setAnimation(&idle);
    animator.reset();
    animator.advanceByTime(1.2F);
    check(std::fabs(animator.phase() - 0.5F) < 0.001F, "idle phase uses duration");
    check(std::fabs(animator.pose().nodes.at("torso").positionOffset.y + 1.0F) < 0.001F,
          "idle samples its keyframe");

    const auto invalidPath = writeFile(
        testDirectory,
        "invalid.json",
        R"({
  "name": "invalid",
  "loop": true,
  "stride_distance": 64.0,
  "duration_seconds": 1.0,
  "keyframes": [
    {"phase":0.5,"nodes":{}},
    {"phase":0.25,"nodes":{}}
  ]
})");
    check(!walk.load(invalidPath, error), "unsorted phases fail");

    std::filesystem::remove_all(testDirectory);
    std::cout << "RigAnimation tests passed.\n";
}
