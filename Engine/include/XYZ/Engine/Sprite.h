#pragma once

#include <SDL3/SDL.h>

namespace xyz::engine {

class Renderer2D;
class Texture;

class Sprite {
public:
    Sprite() = default;
    explicit Sprite(const Texture* texture) noexcept;

    void setTexture(const Texture* texture) noexcept;
    void setDestination(SDL_FRect destination) noexcept;
    void setFlipHorizontal(bool flipHorizontal) noexcept;
    [[nodiscard]] bool render(Renderer2D& renderer) const;

private:
    const Texture* texture_ = nullptr;
    SDL_FRect destination_{0.0F, 0.0F, 0.0F, 0.0F};
    bool flipHorizontal_ = false;
};

}
