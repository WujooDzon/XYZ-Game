#include "XYZ/Engine/Animation.h"

#include <algorithm>
#include <utility>

namespace xyz::engine {

Animation::Animation(std::vector<std::string> frames, float framesPerSecond)
    : frames_(std::move(frames)) {
    setFramesPerSecond(framesPerSecond);
}

void Animation::update(float deltaSeconds) {
    if (frames_.empty() || deltaSeconds <= 0.0F || framesPerSecond_ <= 0.0) {
        return;
    }

    elapsedSeconds_ += static_cast<double>(deltaSeconds);
    const double frameDuration = 1.0 / framesPerSecond_;
    while (elapsedSeconds_ >= frameDuration) {
        elapsedSeconds_ -= frameDuration;
        currentFrame_ = (currentFrame_ + 1) % frames_.size();
    }
}

void Animation::reset() {
    currentFrame_ = 0;
    elapsedSeconds_ = 0.0;
}

void Animation::setFramesPerSecond(float framesPerSecond) {
    framesPerSecond_ = std::max(0.001, static_cast<double>(framesPerSecond));
}

std::size_t Animation::currentFrame() const noexcept {
    return currentFrame_;
}

const std::string& Animation::currentFramePath() const {
    static const std::string emptyPath;
    return frames_.empty() ? emptyPath : frames_[currentFrame_];
}

float Animation::framesPerSecond() const noexcept {
    return static_cast<float>(framesPerSecond_);
}

bool Animation::empty() const noexcept {
    return frames_.empty();
}

}
