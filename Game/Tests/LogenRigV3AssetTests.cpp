#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "XYZ/Engine/Json.h"

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
    SDL_Quit();

    validateManifest(directory / "Logen_rig_v3_manifest.json");
    validateDefinition(directory / "Logen_rig_v3_definition.json");
    validateAnimation(
        directory / "Logen_walk_v3.json",
        {"contact_a", "down_a", "passing_a", "up_a",
         "contact_b", "down_b", "passing_b", "up_b"});
    std::string error;
    const auto idle = JsonValue::parseFile(directory / "Logen_idle_v3.json", error);
    check(idle.has_value() && idle->isObject(), "V3 idle animation parses");
    const JsonValue* idleKeyframes = required(*idle, "keyframes");
    check(idleKeyframes->isArray() && idleKeyframes->array().size() == 2U,
          "V3 idle animation contains two poses");
    for (const JsonValue& keyframe : idleKeyframes->array()) {
        check(!requiredString(keyframe, "label").empty(), "V3 idle poses are labeled");
    }

    std::cout << "Logen Rig V3 asset contract tests passed.\n";
}
