#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace xyz::engine {

class Animation {
public:
    Animation(std::vector<std::string> frames, float framesPerSecond);

    void update(float deltaSeconds);
    void reset();
    void setFramesPerSecond(float framesPerSecond);

    [[nodiscard]] std::size_t currentFrame() const noexcept;
    [[nodiscard]] const std::string& currentFramePath() const;
    [[nodiscard]] float framesPerSecond() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    std::vector<std::string> frames_;
    std::size_t currentFrame_ = 0;
    double elapsedSeconds_ = 0.0;
    double framesPerSecond_ = 1.0;
};

}
