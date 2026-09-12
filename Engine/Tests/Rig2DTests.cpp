#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "XYZ/Engine/Rig2D.h"
#include "XYZ/Engine/Renderer2D.h"

namespace {

std::filesystem::path writeDefinition(
    const std::filesystem::path& directory,
    const std::string& name,
    const std::string& nodes,
    float targetHeight = 941.0F) {
    const std::filesystem::path path = directory / name;
    std::ofstream file(path);
    file << R"({
  "global_scale": 1.0,
  "target_height": )" << targetHeight << R"(,
  "root_to_ground": [0.0, 0.0],
  "nodes": )" << nodes << "\n}\n";
    return path;
}

const xyz::engine::RigWorldNode* findWorldNode(
    const std::vector<xyz::engine::RigWorldNode>& nodes,
    const std::string& id) {
    for (const auto& node : nodes) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

} // namespace

int main() {
    const auto check = [](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "Rig2D test failed: " << message << "\n";
            std::exit(1);
        }
    };

    check(SDL_Init(SDL_INIT_VIDEO), "SDL video initializes");
    SDL_Window* window = SDL_CreateWindow(
        "XYZ Rig2D Test",
        xyz::engine::Renderer2D::LogicalWidth,
        xyz::engine::Renderer2D::LogicalHeight,
        SDL_WINDOW_HIDDEN);
    check(window != nullptr, "hidden window is created");

    const std::filesystem::path testDirectory =
        std::filesystem::temp_directory_path() / "xyz-stage01b-rig2d-tests";
    std::filesystem::create_directories(testDirectory);
    const std::string image = (std::filesystem::current_path()
                               / "Assets" / "Locations" / "GuffmanBasement"
                               / "GuffmanBasement_BG_v1.png").string();
    const std::string validNodes =
        "["
        "{\"id\":\"root\",\"parent\":null,\"image\":\"\",\"position\":[0,0],\"pivot\":[0.5,0.5],\"rotation\":0,\"scale\":[1,1],\"z\":0},"
        "{\"id\":\"parent\",\"parent\":\"root\",\"image\":\"" + image + "\",\"position\":[10,20],\"pivot\":[0.5,0.5],\"rotation\":0,\"scale\":[1,1],\"z\":1},"
        "{\"id\":\"child\",\"parent\":\"parent\",\"image\":\"" + image + "\",\"position\":[5,0],\"pivot\":[0.5,0.5],\"rotation\":0,\"scale\":[1,1],\"z\":2}"
        "]";

    {
        xyz::engine::Renderer2D renderer(window);
        check(renderer.initialized(), "renderer initializes");

        xyz::engine::Rig2D rig;
        std::string error;
        const auto validPath = writeDefinition(testDirectory, "valid.json", validNodes);
        check(rig.loadDefinition(renderer, validPath, error), error.c_str());
        check(rig.nodes().size() == 3, "all nodes load");
        check(rig.nodes()[1].children.size() == 1, "children are linked");
        check(std::fabs(rig.neutralHeight() - 941.0F) < 0.1F,
              "neutral rig height is normalized to target height");
        check(rig.rootToGround().x == 0.0F && rig.rootToGround().y == 0.0F,
              "ground anchor is explicit and fixed");

        const auto paddedImage = testDirectory / "padded-rig.png";
        SDL_Surface* paddedSurface = SDL_CreateSurface(10, 20, SDL_PIXELFORMAT_RGBA32);
        check(paddedSurface != nullptr, "padded rig fixture surface is created");
        check(SDL_ClearSurface(paddedSurface, 0.0F, 0.0F, 0.0F, 0.0F),
              "padded rig fixture starts transparent");
        const SDL_Rect visibleRectangle{2, 4, 6, 10};
        check(SDL_FillSurfaceRect(
                  paddedSurface,
                  &visibleRectangle,
                  SDL_MapSurfaceRGBA(paddedSurface, 70, 50, 30, 255)),
              "padded rig fixture contains visible art");
        check(IMG_SavePNG(paddedSurface, paddedImage.string().c_str()),
              "padded rig fixture is saved");
        SDL_DestroySurface(paddedSurface);
        const std::string paddedNodes =
            "[{\"id\":\"root\",\"parent\":null,\"image\":\""
            + paddedImage.string()
            + "\",\"position\":[0,0],\"pivot\":[0,0],\"rotation\":0,"
              "\"scale\":[1,1],\"z\":0}]";
        xyz::engine::Rig2D paddedRig;
        const auto paddedDefinition = writeDefinition(
            testDirectory, "padded-rig.json", paddedNodes, 100.0F);
        check(paddedRig.loadDefinition(renderer, paddedDefinition, error), error.c_str());
        const SDL_FRect paddedVisibleBounds = paddedRig.visibleBounds({});
        const SDL_FRect paddedCanvasBounds = paddedRig.bounds({});
        check(std::fabs(paddedVisibleBounds.h - 100.0F) < 0.1F,
              "transparent padding does not change normalized visible height");
        check(std::fabs(paddedCanvasBounds.h - 200.0F) < 0.1F,
              "canvas bounds remain distinct from visible art bounds");

        const std::string mirroredNodes =
            "["
            "{\"id\":\"root\",\"parent\":null,\"image\":\"\",\"position\":[0,0],"
            "\"pivot\":[0.5,0.5],\"rotation\":12,\"scale\":[1,1],\"z\":0},"
            "{\"id\":\"boot\",\"parent\":\"root\",\"image\":\""
            + paddedImage.string()
            + "\",\"position\":[20,10],\"pivot\":[0.18,0.12],"
              "\"ground_contact\":[0.90,0.90],\"rotation\":0,"
              "\"scale\":[1.2,0.8],\"z\":1}]";
        xyz::engine::Rig2D mirroredRig;
        const auto mirroredDefinition = writeDefinition(
            testDirectory, "mirrored-contact.json", mirroredNodes, 100.0F);
        check(mirroredRig.loadDefinition(renderer, mirroredDefinition, error), error.c_str());
        constexpr float mirrorAxis = 300.0F;
        mirroredRig.setRootPosition({mirrorAxis, 210.0F});
        for (const float rotation : {0.0F, 25.0F, -25.0F}) {
            xyz::engine::RigPose pose;
            pose.nodes["boot"].rotationDegrees = rotation;
            mirroredRig.setMirrored(false);
            const SDL_FPoint rightContact = mirroredRig.footContactPosition("boot", pose);
            const SDL_FRect rightBounds = mirroredRig.bounds(pose);
            const SDL_FRect rightVisibleBounds = mirroredRig.visibleBounds(pose);
            mirroredRig.setMirrored(true);
            const SDL_FPoint leftContact = mirroredRig.footContactPosition("boot", pose);
            const SDL_FRect leftBounds = mirroredRig.bounds(pose);
            const SDL_FRect leftVisibleBounds = mirroredRig.visibleBounds(pose);
            check(std::fabs(leftContact.x - (2.0F * mirrorAxis - rightContact.x)) < 0.1F,
                  "mirrored p=0.18/c=0.90 contact reflects around the render root");
            check(std::fabs(leftContact.y - rightContact.y) < 0.1F,
                  "mirrored contact preserves Y under parent and child rotations");
            check(std::fabs(leftBounds.x
                            - (2.0F * mirrorAxis - (rightBounds.x + rightBounds.w))) < 0.1F
                      && std::fabs(leftBounds.w - rightBounds.w) < 0.1F
                      && std::fabs(leftBounds.y - rightBounds.y) < 0.1F
                      && std::fabs(leftBounds.h - rightBounds.h) < 0.1F,
                  "mirrored canvas bounds match the rendered texture geometry");
            check(std::fabs(leftVisibleBounds.x
                            - (2.0F * mirrorAxis
                               - (rightVisibleBounds.x + rightVisibleBounds.w))) < 0.1F
                      && std::fabs(leftVisibleBounds.w - rightVisibleBounds.w) < 0.1F,
                  "mirrored visible bounds reflect around the same root");
            mirroredRig.setMirrored(false);
            const SDL_FPoint reflectedTwice = mirroredRig.footContactPosition("boot", pose);
            check(std::fabs(reflectedTwice.x - rightContact.x) < 0.1F
                      && std::fabs(reflectedTwice.y - rightContact.y) < 0.1F,
                  "disabling mirroring restores the original contact");
        }

        rig.setRootPosition({100.0F, 200.0F});
        const auto neutralWorld = rig.worldNodes({});
        const auto* child = findWorldNode(neutralWorld, "child");
        check(child != nullptr, "child world node exists");
        check(child->position.x == 115.0F && child->position.y == 220.0F,
              "child position follows parent translation");

        xyz::engine::RigPose logicalOffsetPose;
        logicalOffsetPose.rootOffsetLogical = {4.0F, -2.0F};
        const auto offsetWorld = rig.worldNodes(logicalOffsetPose);
        const auto* offsetRoot = findWorldNode(offsetWorld, "root");
        child = findWorldNode(offsetWorld, "child");
        check(offsetRoot != nullptr && child != nullptr, "logical offset world nodes exist");
        check(offsetRoot->position.x == 104.0F && offsetRoot->position.y == 198.0F,
              "logical root offset applies before hierarchy evaluation");
        check(child->position.x == 119.0F && child->position.y == 218.0F,
              "logical root offset moves children without scaling the offset");

        xyz::engine::RigPose rotatedPose;
        rotatedPose.nodes["parent"].rotationDegrees = 90.0F;
        const auto rotatedWorld = rig.worldNodes(rotatedPose);
        child = findWorldNode(rotatedWorld, "child");
        check(child != nullptr, "rotated child world node exists");
        check(child->position.x == 110.0F && child->position.y == 225.0F,
              "child position follows parent rotation");

        rig.setMirrored(true);
        const auto mirroredWorld = rig.worldNodes({});
        child = findWorldNode(mirroredWorld, "child");
        check(child != nullptr, "mirrored child world node exists");
        check(child->position.x == 85.0F, "child position mirrors around root");
        rig.setMirrored(false);

        const auto childIndex = rig.nodeIndex("child");
        check(childIndex.has_value(), "calibration can select a node");
        check(rig.adjustNodePosition(*childIndex, {2.0F, -1.0F}),
              "calibration moves a node");
        check(rig.adjustNodeRotation(*childIndex, 3.0F),
              "calibration rotates a node");
        check(rig.adjustNodePivot(*childIndex, {0.01F, -0.01F}),
              "calibration adjusts a pivot");
        check(rig.saveCalibration(error), "calibration saves with a backup");
        check(std::filesystem::exists(validPath.parent_path() / "valid.backup.json"),
              "calibration backup is created before overwrite");

        const std::size_t validNodeCount = rig.nodes().size();
        const auto duplicatePath = writeDefinition(
            testDirectory,
            "duplicate.json",
            "[{\"id\":\"root\",\"parent\":null,\"image\":\"\",\"position\":[0,0],\"pivot\":[0.5,0.5],\"rotation\":0,\"scale\":[1,1],\"z\":0},"
            "{\"id\":\"root\",\"parent\":\"root\",\"image\":\"\",\"position\":[0,0],\"pivot\":[0.5,0.5],\"rotation\":0,\"scale\":[1,1],\"z\":1}]");
        check(!rig.loadDefinition(renderer, duplicatePath, error), "duplicate ids fail");
        check(rig.nodes().size() == validNodeCount, "failed load preserves valid rig");

        const auto badPivotPath = writeDefinition(
            testDirectory,
            "bad-pivot.json",
            "[{\"id\":\"root\",\"parent\":null,\"image\":\"\",\"position\":[0,0],\"pivot\":[1.2,0.5],\"rotation\":0,\"scale\":[1,1],\"z\":0}]");
        check(!rig.loadDefinition(renderer, badPivotPath, error), "invalid pivot fails");

        const auto missingParentPath = writeDefinition(
            testDirectory,
            "missing-parent.json",
            "[{\"id\":\"root\",\"parent\":null,\"image\":\"\",\"position\":[0,0],\"pivot\":[0.5,0.5],\"rotation\":0,\"scale\":[1,1],\"z\":0},"
            "{\"id\":\"child\",\"parent\":\"missing\",\"image\":\"\",\"position\":[0,0],\"pivot\":[0.5,0.5],\"rotation\":0,\"scale\":[1,1],\"z\":1}]");
        check(!rig.loadDefinition(renderer, missingParentPath, error), "missing parent fails");

        std::filesystem::remove_all(testDirectory);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "Rig2D tests passed.\n";
}
