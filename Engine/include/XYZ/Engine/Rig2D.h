#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "XYZ/Engine/Texture.h"

namespace xyz::engine {

struct RigPoseTransform {
    SDL_FPoint positionOffset{0.0F, 0.0F};
    float rotationDegrees = 0.0F;
    SDL_FPoint scaleMultiplier{1.0F, 1.0F};
};

struct RigPose {
    std::map<std::string, RigPoseTransform> nodes;
};

struct RigNode {
    std::string id;
    int parentIndex = -1;
    std::vector<int> children;
    Texture texture;
    SDL_FPoint localPosition{0.0F, 0.0F};
    SDL_FPoint pivot{0.5F, 0.5F};
    float baseRotationDegrees = 0.0F;
    SDL_FPoint localScale{1.0F, 1.0F};
    int zOrder = 0;
};

struct RigWorldNode {
    int nodeIndex = -1;
    int parentIndex = -1;
    std::string id;
    SDL_FPoint position{0.0F, 0.0F};
    float rotationDegrees = 0.0F;
    SDL_FPoint scale{1.0F, 1.0F};
    SDL_FPoint pivot{0.5F, 0.5F};
    SDL_FRect bounds{0.0F, 0.0F, 0.0F, 0.0F};
    int zOrder = 0;
};

class Renderer2D;

class Rig2D {
public:
    bool loadDefinition(
        Renderer2D& renderer,
        const std::filesystem::path& definitionPath,
        std::string& error);

    void setRootPosition(SDL_FPoint baselineAnchor) noexcept;
    void setMirrored(bool mirrored) noexcept;

    [[nodiscard]] bool render(Renderer2D& renderer, const RigPose& pose) const;
    [[nodiscard]] bool debugRender(Renderer2D& renderer, const RigPose& pose) const;
    [[nodiscard]] std::vector<RigWorldNode> worldNodes(const RigPose& pose) const;
    [[nodiscard]] SDL_FRect bounds(const RigPose& pose) const;

    [[nodiscard]] const std::vector<RigNode>& nodes() const noexcept;
    [[nodiscard]] float globalScale() const noexcept;
    [[nodiscard]] float targetHeight() const noexcept;
    [[nodiscard]] const std::filesystem::path& definitionPath() const noexcept;
    [[nodiscard]] bool mirrored() const noexcept;

private:
    struct DefinitionSettings {
        float globalScale = 1.0F;
        float targetHeight = 0.0F;
        SDL_FPoint rootOffset{0.0F, 0.0F};
    };

    std::vector<RigWorldNode> evaluateWorldNodes(const RigPose& pose) const;
    void evaluateNode(
        int nodeIndex,
        const RigPose& pose,
        const SDL_FPoint& parentPosition,
        float parentRotationDegrees,
        SDL_FPoint parentScale,
        bool hasParent,
        std::vector<RigWorldNode>& output) const;

    std::vector<RigNode> nodes_;
    int rootIndex_ = -1;
    DefinitionSettings settings_;
    SDL_FPoint baselineAnchor_{0.0F, 0.0F};
    bool mirrored_ = false;
    std::filesystem::path definitionPath_;
};

}
