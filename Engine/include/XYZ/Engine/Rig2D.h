#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
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
    SDL_FPoint rootOffsetLogical{0.0F, 0.0F};
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
    SDL_FPoint groundContact{0.5F, 1.0F};
    int zOrder = 0;
    std::string imagePath;
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
    void setVisualRootCorrectionX(float correction) noexcept;
    void setMirrored(bool mirrored) noexcept;

    [[nodiscard]] bool render(Renderer2D& renderer, const RigPose& pose) const;
    [[nodiscard]] bool debugRender(Renderer2D& renderer, const RigPose& pose) const;
    [[nodiscard]] std::vector<RigWorldNode> worldNodes(const RigPose& pose) const;
    [[nodiscard]] SDL_FRect bounds(const RigPose& pose) const;
    [[nodiscard]] SDL_FPoint footContactPosition(
        std::string_view nodeId,
        const RigPose& pose) const;

    [[nodiscard]] const std::vector<RigNode>& nodes() const noexcept;
    [[nodiscard]] std::optional<std::size_t> nodeIndex(std::string_view id) const noexcept;
    [[nodiscard]] bool adjustNodePosition(std::size_t index, SDL_FPoint delta) noexcept;
    [[nodiscard]] bool adjustNodeRotation(std::size_t index, float deltaDegrees) noexcept;
    [[nodiscard]] bool adjustNodePivot(std::size_t index, SDL_FPoint delta) noexcept;
    [[nodiscard]] bool saveCalibration(std::string& error) const;
    [[nodiscard]] float globalScale() const noexcept;
    [[nodiscard]] float neutralHeight() const noexcept;
    [[nodiscard]] float targetHeight() const noexcept;
    [[nodiscard]] SDL_FPoint rootToGround() const noexcept;
    [[nodiscard]] const std::filesystem::path& definitionPath() const noexcept;
    [[nodiscard]] bool mirrored() const noexcept;

private:
    struct DefinitionSettings {
        float globalScaleMultiplier = 1.0F;
        float targetHeight = 0.0F;
        SDL_FPoint rootToGround{0.0F, 0.0F};
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
    SDL_FPoint gameplayRoot_{0.0F, 0.0F};
    float visualRootCorrectionX_ = 0.0F;
    float neutralHeight_ = 0.0F;
    float effectiveScale_ = 1.0F;
    bool mirrored_ = false;
    std::filesystem::path definitionPath_;
};

}
