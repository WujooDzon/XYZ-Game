#include "XYZ/Engine/Texture.h"

#include <iostream>
#include <utility>

#include <SDL3_image/SDL_image.h>

namespace xyz::engine {

Texture::~Texture() {
    reset();
}

Texture::Texture(Texture&& other) noexcept
    : texture_(other.texture_), width_(other.width_), height_(other.height_) {
    other.texture_ = nullptr;
    other.width_ = 0.0F;
    other.height_ = 0.0F;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    reset();
    texture_ = other.texture_;
    width_ = other.width_;
    height_ = other.height_;
    other.texture_ = nullptr;
    other.width_ = 0.0F;
    other.height_ = 0.0F;
    return *this;
}

bool Texture::load(SDL_Renderer* renderer, const std::filesystem::path& path, const char* label) {
    reset();

    texture_ = IMG_LoadTexture(renderer, path.string().c_str());
    if (texture_ == nullptr) {
        std::cerr << "Could not load " << (label == nullptr ? "texture" : label)
                  << " '" << path.string() << "': " << SDL_GetError() << "\n";
        return false;
    }

    if (!SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_NEAREST)) {
        std::cerr << "Could not set nearest-neighbor sampling for '" << path.string()
                  << "': " << SDL_GetError() << "\n";
        reset();
        return false;
    }

    if (!SDL_GetTextureSize(texture_, &width_, &height_)) {
        std::cerr << "Could not read texture dimensions for '" << path.string()
                  << "': " << SDL_GetError() << "\n";
        reset();
        return false;
    }

    return true;
}

void Texture::reset() noexcept {
    if (texture_ != nullptr) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    width_ = 0.0F;
    height_ = 0.0F;
}

bool Texture::loaded() const noexcept {
    return texture_ != nullptr;
}

SDL_Texture* Texture::native() const noexcept {
    return texture_;
}

float Texture::width() const noexcept {
    return width_;
}

float Texture::height() const noexcept {
    return height_;
}

}
