#pragma once

#include <filesystem>
#include <string>

namespace xyz::engine {
class Renderer2D;
class Rig2D;
class RigAnimation;
}

namespace xyz::game {

class RigReviewExporter final {
public:
    bool exportWalkReview(
        engine::Renderer2D& renderer,
        engine::Rig2D& rig,
        const engine::RigAnimation& idleAnimation,
        const engine::RigAnimation& walkAnimation,
        const std::filesystem::path& outputDirectory,
        std::string& error) const;
};

}
