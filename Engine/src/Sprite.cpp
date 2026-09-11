#include "XYZ/Engine/Sprite.h"

#include "XYZ/Engine/Renderer2D.h"
#include "XYZ/Engine/Texture.h"

namespace xyz::engine {

Sprite::Sprite(const Texture* texture) noexcept
    : texture_(texture) {}

void Sprite::setTexture(const Texture* texture) noexcept {
    texture_ = texture;
}

void Sprite::setDestination(SDL_FRect destination) noexcept {
    destination_ = destination;
}

void Sprite::setFlipHorizontal(bool flipHorizontal) noexcept {
    flipHorizontal_ = flipHorizontal;
}

bool Sprite::render(Renderer2D& renderer) const {
    return texture_ != nullptr && renderer.drawTexture(*texture_, destination_, flipHorizontal_);
}

}
