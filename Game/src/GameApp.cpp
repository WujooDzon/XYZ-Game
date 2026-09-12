#include "XYZ/Game/GameApp.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "XYZ/Engine/Application.h"
#include "XYZ/Engine/Json.h"
#include "XYZ/Engine/RigAnimation.h"
#include "XYZ/Game/GuffmanBasementScene.h"

namespace xyz::game {
namespace {

bool isProjectRoot(const std::filesystem::path& candidate) {
    return std::filesystem::exists(candidate / "CMakeLists.txt")
        && std::filesystem::is_directory(candidate / "Assets");
}

std::filesystem::path firstProjectRootFrom(std::filesystem::path candidate) {
    std::error_code error;
    candidate = std::filesystem::weakly_canonical(candidate, error);
    if (error) {
        candidate = candidate.lexically_normal();
    }
    if (!std::filesystem::is_directory(candidate)) {
        candidate = candidate.parent_path();
    }

    while (!candidate.empty()) {
        if (isProjectRoot(candidate)) {
            return candidate;
        }
        const std::filesystem::path parent = candidate.parent_path();
        if (parent == candidate) {
            break;
        }
        candidate = parent;
    }
    return {};
}

std::vector<std::filesystem::path> stage01DAssetPaths(const std::filesystem::path& root) {
    const std::filesystem::path characterDirectory = root / "Assets" / "Characters" / "Logen";
    const std::filesystem::path rigDirectory = characterDirectory / "RigV3";
    std::vector<std::filesystem::path> paths{
        root / "Assets" / "Locations" / "GuffmanBasement" / "GuffmanBasement_BG_v1.png",
        characterDirectory / "Logen_Master_Right_v1.png",
        rigDirectory / "Logen_rig_v3_manifest.json",
        rigDirectory / "Logen_rig_v3_definition.json",
        rigDirectory / "Logen_walk_v3.json",
        rigDirectory / "Logen_idle_v3.json"};

    const std::vector<std::string> rigParts{
        "Logen_rig_v3_body_shell.png",
        "Logen_rig_v3_left_arm.png",
        "Logen_rig_v3_cloak_tail.png",
        "Logen_rig_v3_cloak_front.png",
        "Logen_rig_v3_far_thigh.png",
        "Logen_rig_v3_far_shin.png",
        "Logen_rig_v3_far_boot.png",
        "Logen_rig_v3_near_thigh.png",
        "Logen_rig_v3_near_shin.png",
        "Logen_rig_v3_near_boot.png"};
    for (const std::string& filename : rigParts) {
        paths.push_back(rigDirectory / filename);
    }
    return paths;
}

} // namespace

GameApp::GameApp(std::filesystem::path projectRoot)
    : projectRoot_(std::move(projectRoot)) {}

std::filesystem::path GameApp::discoverProjectRoot(const char* executablePath) {
    if (const char* configuredRoot = std::getenv("XYZ_PROJECT_ROOT"); configuredRoot != nullptr) {
        if (const auto root = firstProjectRootFrom(configuredRoot); !root.empty()) {
            return root;
        }
    }

    if (const auto root = firstProjectRootFrom(std::filesystem::current_path()); !root.empty()) {
        return root;
    }

    if (executablePath != nullptr && executablePath[0] != '\0') {
        if (const auto root = firstProjectRootFrom(executablePath); !root.empty()) {
            return root;
        }
    }

    return std::filesystem::current_path();
}

int GameApp::run(int argc, char** argv) const {
    for (int index = 1; index < argc; ++index) {
        if (std::string(argv[index]) == "--self-test") {
            return runSelfTest();
        }
    }

    GuffmanBasementScene scene(projectRoot_);
    engine::Application application("XYZ Game - Guffman's Basement");
    return application.run(scene);
}

int GameApp::runSelfTest() const {
    for (const auto& path : stage01DAssetPaths(projectRoot_)) {
        if (!std::filesystem::is_regular_file(path)) {
            std::cerr << "Stage 01D self-test missing Rig V3 asset: " << path.string() << "\n";
            return 1;
        }
    }

    const std::filesystem::path rigDirectory =
        projectRoot_ / "Assets" / "Characters" / "Logen" / "RigV3";
    std::string error;
    const auto parseObject = [&](const std::filesystem::path& path,
                                 const char* label) -> std::optional<engine::JsonValue> {
        auto document = engine::JsonValue::parseFile(path, error);
        if (!document.has_value() || !document->isObject()) {
            std::cerr << "Stage 01D self-test invalid Rig V3 " << label << ": "
                      << (error.empty() ? path.string() : error) << "\n";
            return std::nullopt;
        }
        return document;
    };

    const auto definition = parseObject(
        rigDirectory / "Logen_rig_v3_definition.json", "rig definition");
    if (!definition.has_value()) {
        return 1;
    }

    const auto* nodeValues = definition->find("nodes");
    const auto* targetHeight = definition->find("target_height");
    const auto* rootToGround = definition->find("root_to_ground");
    if (targetHeight == nullptr || !targetHeight->isNumber()
        || std::fabs(targetHeight->number() - 188.0) > 0.001
        || rootToGround == nullptr || !rootToGround->isArray()
        || rootToGround->array().size() != 2U) {
        std::cerr << "Stage 01D self-test requires Rig V3 target_height 188 and root_to_ground\n";
        return 1;
    }
    const std::set<std::string> expectedParts{
        "Logen_rig_v3_body_shell.png",
        "Logen_rig_v3_left_arm.png",
        "Logen_rig_v3_cloak_tail.png",
        "Logen_rig_v3_cloak_front.png",
        "Logen_rig_v3_far_thigh.png",
        "Logen_rig_v3_far_shin.png",
        "Logen_rig_v3_far_boot.png",
        "Logen_rig_v3_near_thigh.png",
        "Logen_rig_v3_near_shin.png",
        "Logen_rig_v3_near_boot.png"};
    std::set<std::string> actualParts;
    bool hasPelvisRoot = false;
    bool hasBodyShell = false;
    if (nodeValues == nullptr || !nodeValues->isArray()
        || nodeValues->array().size() != expectedParts.size() + 1U) {
        std::cerr << "Stage 01D self-test expected Rig V3 pelvis plus 10 active rig parts\n";
        return 1;
    }
    for (const auto& node : nodeValues->array()) {
        const auto* id = node.find("id");
        const auto* image = node.find("image");
        if (!node.isObject() || id == nullptr || image == nullptr
            || !id->isString() || !image->isString()) {
            std::cerr << "Stage 01D self-test found malformed rig node\n";
            return 1;
        }
        if (id->string() == "pelvis") {
            hasPelvisRoot = image->string().empty();
            continue;
        }
        const std::string imageName = std::filesystem::path(image->string()).filename().string();
        actualParts.insert(imageName);
        if (id->string() == "body_shell"
            && imageName == "Logen_rig_v3_body_shell.png") {
            hasBodyShell = true;
        }
        if (id->string().find("right_hand") != std::string::npos
            || imageName.find("right_hand") != std::string::npos
            || imageName.find("right_forearm") != std::string::npos) {
            std::cerr << "Stage 01D self-test found forbidden right hand art\n";
            return 1;
        }
    }
    if (!hasPelvisRoot || !hasBodyShell || actualParts != expectedParts) {
        std::cerr << "Stage 01D self-test Rig V3 nodes do not match active parts\n";
        return 1;
    }

    const auto walk = parseObject(rigDirectory / "Logen_walk_v3.json", "walk animation");
    const auto idle = parseObject(rigDirectory / "Logen_idle_v3.json", "idle animation");
    if (!walk.has_value() || !idle.has_value()) {
        return 1;
    }
    const auto* walkName = walk->find("name");
    const auto* walkKeyframes = walk->find("keyframes");
    if (walkName == nullptr || !walkName->isString() || walkName->string() != "walk"
        || walkKeyframes == nullptr || !walkKeyframes->isArray()
        || walkKeyframes->array().size() != 8U) {
        std::cerr << "Stage 01D self-test walk animation must contain eight labeled poses\n";
        return 1;
    }
    const std::array<const char*, 8> expectedWalkLabels{
        "contact_a", "down_a", "passing_a", "up_a",
        "contact_b", "down_b", "passing_b", "up_b"};
    const auto* walkStride = walk->find("stride_distance");
    if (walkStride == nullptr || !walkStride->isNumber()
        || walkStride->number() < 55.0 || walkStride->number() > 80.0) {
        std::cerr << "Stage 01D self-test walk stride must be tuned to 55..80 logical px\n";
        return 1;
    }
    for (std::size_t index = 0; index < walkKeyframes->array().size(); ++index) {
        const auto& keyframe = walkKeyframes->array()[index];
        const auto* label = keyframe.find("label");
        const auto* phase = keyframe.find("phase");
        const auto* rootOffset = keyframe.find("root_offset_px");
        if (label == nullptr || !label->isString()
            || label->string() != expectedWalkLabels[index]
            || rootOffset == nullptr || !rootOffset->isArray()
            || rootOffset->array().size() != 2U
            || !rootOffset->array()[0].isNumber()
            || !rootOffset->array()[1].isNumber()
            || std::fabs(rootOffset->array()[0].number()) > 0.001
            || !std::isfinite(rootOffset->array()[1].number())
            || std::fabs(rootOffset->array()[1].number()) > 8.0) {
            std::cerr << "Stage 01D self-test walk labels/root offsets are invalid\n";
            return 1;
        }
        if (phase == nullptr || !phase->isNumber()
            || phase->number() != static_cast<double>(index) / 8.0) {
            std::cerr << "Stage 01D self-test walk phases are not 0..0.875\n";
            return 1;
        }
    }
    const auto* idleName = idle->find("name");
    const auto* idleDuration = idle->find("duration_seconds");
    const auto* idleKeyframes = idle->find("keyframes");
    if (idleName == nullptr || !idleName->isString() || idleName->string() != "idle"
        || idleDuration == nullptr || !idleDuration->isNumber() || idleDuration->number() <= 0.0
        || idleKeyframes == nullptr || !idleKeyframes->isArray()
        || idleKeyframes->array().size() < 2U) {
        std::cerr << "Stage 01D self-test idle animation is incomplete\n";
        return 1;
    }

    engine::RigAnimation walkAnimation;
    engine::RigAnimation idleAnimation;
    if (!walkAnimation.load(rigDirectory / "Logen_walk_v3.json", error)
        || !idleAnimation.load(rigDirectory / "Logen_idle_v3.json", error)) {
        std::cerr << "Stage 01D self-test animation load failed: " << error << "\n";
        return 1;
    }

    std::cout << "XYZ Game Stage 01D self-test passed.\n";
    return 0;
}

}
