#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "XYZ/Engine/Json.h"
#include "XYZ/Engine/Rig2D.h"
#include "XYZ/Engine/RigAnimation.h"
#include "XYZ/Engine/Renderer2D.h"

namespace {

using xyz::engine::JsonValue;

constexpr std::array<std::string_view, 10> kExpectedParts{
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

struct AlphaBounds {
    int left = 0;
    int top = 0;
    int right = -1;
    int bottom = -1;
};

const xyz::engine::RigWorldNode* findWorldNode(
    const std::vector<xyz::engine::RigWorldNode>& nodes,
    std::string_view id) {
    for (const auto& node : nodes) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "LogenRigV3Asset test failed: " << message << "\n";
        std::exit(1);
    }
}

const JsonValue* required(const JsonValue& object, std::string_view key) {
    const JsonValue* value = object.find(key);
    check(value != nullptr, "missing JSON key: " + std::string(key));
    return value;
}

std::string requiredString(const JsonValue& object, std::string_view key) {
    const JsonValue* value = required(object, key);
    check(value->isString(), "JSON key is not a string: " + std::string(key));
    return value->string();
}

float requiredNumber(const JsonValue& object, std::string_view key) {
    const JsonValue* value = required(object, key);
    check(value->isNumber(), "JSON key is not a number: " + std::string(key));
    return static_cast<float>(value->number());
}

float requiredRotation(const JsonValue& keyframe, std::string_view nodeId) {
    const JsonValue* nodes = required(keyframe, "nodes");
    check(nodes->isObject(), "keyframe nodes are an object");
    const JsonValue* node = required(*nodes, nodeId);
    check(node->isObject(), "keyframe node is an object: " + std::string(nodeId));
    return requiredNumber(*node, "rotation");
}

void validateRootOffset(const JsonValue& keyframe, float expectedY) {
    const JsonValue* offset = required(keyframe, "root_offset_px");
    check(offset->isArray() && offset->array().size() == 2U,
          "walk keyframe has a logical root offset");
    check(offset->array()[0].isNumber() && offset->array()[1].isNumber(),
          "walk root offset contains numbers");
    check(std::fabs(offset->array()[0].number()) < 0.001
              && std::fabs(offset->array()[1].number() - expectedY) < 0.001,
          "walk root offset matches the requested body-weight bob");
}

void validateWalkBiomechanics(const JsonValue& animation) {
    const JsonValue* keyframes = required(animation, "keyframes");
    check(keyframes->isArray() && keyframes->array().size() == 8U,
          "walk has exactly eight keyframes");
    const std::array<float, 8> rootOffsets{0.0F, 2.0F, 1.0F, -1.0F,
                                           0.0F, 2.0F, 1.0F, -1.0F};
    for (std::size_t index = 0; index < rootOffsets.size(); ++index) {
        validateRootOffset(keyframes->array()[index], rootOffsets[index]);
        check(std::fabs(requiredRotation(keyframes->array()[index], "body_shell")) <= 0.35F,
              "body shell stays visually stable during the walk");
        check(std::fabs(requiredRotation(keyframes->array()[index], "cloak_tail")) <= 0.5F,
              "cloak tail motion stays secondary");
        check(std::fabs(requiredRotation(keyframes->array()[index], "cloak_front")) <= 0.5F,
              "cloak front motion stays secondary");
    }

    const JsonValue& contactA = keyframes->array()[0];
    const JsonValue& passingA = keyframes->array()[2];
    const JsonValue& contactB = keyframes->array()[4];
    const JsonValue& passingB = keyframes->array()[6];
    check(requiredRotation(contactA, "far_thigh") <= -18.0F
              && requiredRotation(contactA, "near_thigh") >= 15.0F,
          "contact A separates far-forward and near-rear legs");
    check(requiredRotation(contactB, "near_thigh") <= -18.0F
              && requiredRotation(contactB, "far_thigh") >= 15.0F,
          "contact B reverses the leg separation");
    check(requiredRotation(passingA, "near_thigh") < 0.0F
              && requiredRotation(passingA, "near_shin") >= 40.0F
              && requiredRotation(passingA, "far_shin") <= 15.0F,
          "passing A has a bent airborne near swing leg");
    check(requiredRotation(passingB, "far_thigh") < 0.0F
              && requiredRotation(passingB, "far_shin") >= 40.0F
              && requiredRotation(passingB, "near_shin") <= 15.0F,
          "passing B has a bent airborne far swing leg");
}

AlphaBounds alphaBounds(SDL_Surface* source) {
    SDL_Surface* converted = SDL_ConvertSurface(source, SDL_PIXELFORMAT_RGBA32);
    check(converted != nullptr, "surface converts to RGBA32");
    const SDL_PixelFormatDetails* details =
        SDL_GetPixelFormatDetails(converted->format);
    check(details != nullptr, "RGBA32 pixel format details are available");

    AlphaBounds bounds{converted->w, converted->h, -1, -1};
    const auto* pixels = static_cast<const Uint8*>(converted->pixels);
    for (int y = 0; y < converted->h; ++y) {
        const auto* row = reinterpret_cast<const Uint32*>(
            pixels + static_cast<std::size_t>(y) * static_cast<std::size_t>(converted->pitch));
        for (int x = 0; x < converted->w; ++x) {
            Uint8 alpha = 0;
            SDL_GetRGBA(row[x], details, nullptr, nullptr, nullptr, nullptr, &alpha);
            if (alpha == 0) {
                continue;
            }
            bounds.left = std::min(bounds.left, x);
            bounds.top = std::min(bounds.top, y);
            bounds.right = std::max(bounds.right, x);
            bounds.bottom = std::max(bounds.bottom, y);
        }
    }
    SDL_DestroySurface(converted);
    return bounds;
}

void validateManifest(const std::filesystem::path& path) {
    std::string error;
    const auto manifest = JsonValue::parseFile(path, error);
    check(manifest.has_value() && manifest->isObject(), "V3 manifest parses");
    check(requiredString(*manifest, "reference") == "../Logen_Master_Right_v1.png",
          "manifest uses the approved master reference");
    check(requiredString(*manifest, "camera") == "side_profile_90",
          "manifest declares exact side-profile camera");
    check(requiredString(*manifest, "facing") == "right",
          "manifest declares right-facing art");

    const JsonValue* parts = required(*manifest, "parts");
    check(parts->isArray() && parts->array().size() == kExpectedParts.size(),
          "manifest contains exactly ten parts");
    for (std::size_t index = 0; index < kExpectedParts.size(); ++index) {
        check(parts->array()[index].isObject(), "manifest part is an object");
        check(requiredString(parts->array()[index], "filename") == kExpectedParts[index],
              "manifest part order matches the V3 contract");
        const JsonValue* overlap = required(parts->array()[index], "overlap_px");
        check(overlap->isNumber() && overlap->number() >= 20.0,
              "manifest records a joint overlap of at least 20 source pixels");
    }
}

void validateDefinition(const std::filesystem::path& path) {
    std::string error;
    const auto definition = JsonValue::parseFile(path, error);
    check(definition.has_value() && definition->isObject(), "V3 definition parses");
    const JsonValue* targetHeight = required(*definition, "target_height");
    check(targetHeight->isNumber() && targetHeight->number() == 188.0,
          "V3 definition targets 188 logical pixels");
    const JsonValue* rootToGround = required(*definition, "root_to_ground");
    check(rootToGround->isArray() && rootToGround->array().size() == 2U,
          "V3 definition contains root_to_ground");
    const JsonValue* nodes = required(*definition, "nodes");
    check(nodes->isArray() && nodes->array().size() == 11U,
          "V3 definition contains pelvis plus ten art nodes");

    const std::set<std::string> expectedIds{
        "pelvis", "body_shell", "left_arm", "cloak_tail", "cloak_front",
        "far_thigh", "far_shin", "far_boot",
        "near_thigh", "near_shin", "near_boot"};
    std::set<std::string> actualIds;
    for (const JsonValue& node : nodes->array()) {
        check(node.isObject(), "V3 node is an object");
        const std::string id = requiredString(node, "id");
        actualIds.insert(id);
        if (id == "body_shell") {
            check(requiredString(node, "role") == "stable_shell_empty_right_sleeve",
                  "body shell owns the empty right sleeve");
        }
        if (id == "far_boot" || id == "near_boot") {
            const JsonValue* contact = required(node, "ground_contact");
            check(contact->isArray() && contact->array().size() == 2U,
                  "each boot defines a ground contact");
        }
        if (id == "pelvis") {
            continue;
        }
        const JsonValue* overlap = required(node, "overlap_px");
        check(overlap->isNumber() && overlap->number() >= 20.0,
              "each articulated V3 node records overlap metadata");
    }
    check(actualIds == expectedIds, "V3 node IDs match the exact hierarchy contract");
}

void validateAnimation(
    const std::filesystem::path& path,
    const std::array<std::string_view, 8>& labels) {
    std::string error;
    const auto animation = JsonValue::parseFile(path, error);
    check(animation.has_value() && animation->isObject(), "V3 animation parses");
    const JsonValue* keyframes = required(*animation, "keyframes");
    check(keyframes->isArray() && keyframes->array().size() == labels.size(),
          "V3 walk animation contains eight poses");
    for (std::size_t index = 0; index < labels.size(); ++index) {
        check(requiredString(keyframes->array()[index], "label") == labels[index],
              "V3 walk labels are ordered contact/down/passing/up");
    }
}

} // namespace

int main() {
    const std::filesystem::path root = std::filesystem::current_path();
    const std::filesystem::path directory =
        root / "Assets" / "Characters" / "Logen" / "RigV3";
    check(std::filesystem::is_directory(directory), "V3 asset directory exists");

    std::set<std::string> actualPngs;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".png") {
            actualPngs.insert(entry.path().filename().string());
        }
    }
    check(actualPngs.size() == kExpectedParts.size(), "V3 directory has exactly ten PNGs");
    check(actualPngs == std::set<std::string>(
                           kExpectedParts.begin(), kExpectedParts.end()),
          "V3 directory has no extra PNG assets");

    check(SDL_Init(SDL_INIT_VIDEO), "SDL video initializes");
    for (const std::string_view filename : kExpectedParts) {
        const std::filesystem::path path = directory / filename;
        check(std::filesystem::is_regular_file(path), "V3 PNG exists: " + std::string(filename));
        SDL_Surface* surface = IMG_Load(path.string().c_str());
        check(surface != nullptr, "V3 PNG loads: " + std::string(filename));
        check(surface->w > 0 && surface->h > 0, "V3 PNG has dimensions: " + std::string(filename));
        check(SDL_ISPIXELFORMAT_ALPHA(surface->format),
              "V3 PNG has an alpha-capable format: " + std::string(filename));
        const AlphaBounds bounds = alphaBounds(surface);
        check(bounds.right >= bounds.left && bounds.bottom >= bounds.top,
              "V3 PNG has visible pixels: " + std::string(filename));
        check(bounds.left >= 8 && bounds.top >= 8
                  && bounds.right < surface->w - 8
                  && bounds.bottom < surface->h - 8,
              "V3 PNG keeps transparent padding around the artwork: " + std::string(filename));
        SDL_DestroySurface(surface);
    }
    validateManifest(directory / "Logen_rig_v3_manifest.json");
    validateDefinition(directory / "Logen_rig_v3_definition.json");
    validateAnimation(
        directory / "Logen_walk_v3.json",
        {"contact_a", "down_a", "passing_a", "up_a",
         "contact_b", "down_b", "passing_b", "up_b"});
    std::string walkError;
    const auto walk = JsonValue::parseFile(directory / "Logen_walk_v3.json", walkError);
    check(walk.has_value() && walk->isObject(), "V3 walk animation parses for biomechanics checks");
    validateWalkBiomechanics(*walk);
    std::string error;
    const auto idle = JsonValue::parseFile(directory / "Logen_idle_v3.json", error);
    check(idle.has_value() && idle->isObject(), "V3 idle animation parses");
    const JsonValue* idleKeyframes = required(*idle, "keyframes");
    check(idleKeyframes->isArray() && idleKeyframes->array().size() == 2U,
          "V3 idle animation contains two poses");
    for (const JsonValue& keyframe : idleKeyframes->array()) {
        check(!requiredString(keyframe, "label").empty(), "V3 idle poses are labeled");
    }

    SDL_Window* window = SDL_CreateWindow(
        "XYZ Logen Rig V3 Asset Test",
        xyz::engine::Renderer2D::LogicalWidth,
        xyz::engine::Renderer2D::LogicalHeight,
        SDL_WINDOW_HIDDEN);
    check(window != nullptr, "hidden window is created for normalized rig checks");
    {
        xyz::engine::Renderer2D renderer(window);
        check(renderer.initialized(), "renderer is created for normalized rig checks");
        xyz::engine::Rig2D rig;
        std::string rigError;
        check(rig.loadDefinition(renderer, directory / "Logen_rig_v3_definition.json", rigError),
              rigError.c_str());
        rig.setRootPosition({0.0F, 0.0F});
        const auto worlds = rig.worldNodes({});
        const auto* farHip = findWorldNode(worlds, "far_thigh");
        const auto* nearHip = findWorldNode(worlds, "near_thigh");
        check(farHip != nullptr && nearHip != nullptr, "far and near hip pivots exist");
        const float hipSeparation = std::fabs(nearHip->position.x - farHip->position.x);
        check(hipSeparation >= 3.0F && hipSeparation <= 5.0F,
              "final normalized hip separation is 3-5 logical pixels: "
                  + std::to_string(hipSeparation));

        xyz::engine::RigAnimation walkAnimation;
        std::string animationError;
        check(walkAnimation.load(directory / "Logen_walk_v3.json", animationError),
              animationError.c_str());
        rig.setRootPosition({0.0F, 500.0F});
        const auto contactA = walkAnimation.sample(walkAnimation.keyframePhase(0));
        const auto passingA = walkAnimation.sample(walkAnimation.keyframePhase(2));
        const auto contactB = walkAnimation.sample(walkAnimation.keyframePhase(4));
        const auto passingB = walkAnimation.sample(walkAnimation.keyframePhase(6));
        const float contactAFarY = rig.footContactPosition("far_boot", contactA).y;
        const float contactANearY = rig.footContactPosition("near_boot", contactA).y;
        const float contactBNearY = rig.footContactPosition("near_boot", contactB).y;
        const float contactBFarY = rig.footContactPosition("far_boot", contactB).y;
        const float passingANearY = rig.footContactPosition("near_boot", passingA).y;
        const float passingAFarY = rig.footContactPosition("far_boot", passingA).y;
        const float passingBNearY = rig.footContactPosition("near_boot", passingB).y;
        const float passingBFarY = rig.footContactPosition("far_boot", passingB).y;
        check(std::fabs(contactAFarY - contactANearY) <= 8.0F,
              "contact A keeps both feet on the same floor line");
        check(std::fabs(contactBNearY - contactBFarY) <= 8.0F,
              "contact B keeps both feet on the same floor line");
        check(passingANearY <= passingAFarY - 4.0F,
              "passing A lifts near foot above the planted far foot");
        check(std::fabs(passingAFarY - contactAFarY) <= 8.0F,
              "passing A planted far foot remains near the floor line");
        check(passingBFarY <= passingBNearY - 4.0F,
              "passing B lifts far foot above the planted near foot");
        check(std::fabs(passingBNearY - contactBNearY) <= 8.0F,
              "passing B planted near foot remains near the floor line");
    }
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Logen Rig V3 asset contract tests passed.\n";
}
