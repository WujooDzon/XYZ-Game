#include "XYZ/Engine/Texture.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <utility>

#include <SDL3_image/SDL_image.h>

namespace xyz::engine {

Texture::~Texture() {
    reset();
}

Texture::Texture(Texture&& other) noexcept
    : texture_(other.texture_),
      width_(other.width_),
      height_(other.height_),
      visibleBounds_(other.visibleBounds_) {
    other.texture_ = nullptr;
    other.width_ = 0.0F;
    other.height_ = 0.0F;
    other.visibleBounds_.reset();
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    reset();
    texture_ = other.texture_;
    width_ = other.width_;
    height_ = other.height_;
    visibleBounds_ = other.visibleBounds_;
    other.texture_ = nullptr;
    other.width_ = 0.0F;
    other.height_ = 0.0F;
    other.visibleBounds_.reset();
    return *this;
}

bool Texture::load(SDL_Renderer* renderer, const std::filesystem::path& path, const char* label) {
    reset();

    SDL_Surface* source = IMG_Load(path.string().c_str());
    if (source == nullptr) {
        std::cerr << "Could not load " << (label == nullptr ? "texture" : label)
                  << " '" << path.string() << "': " << SDL_GetError() << "\n";
        return false;
    }

    SDL_Surface* pixels = SDL_ConvertSurface(source, SDL_PIXELFORMAT_RGBA32);
    if (pixels == nullptr) {
        std::cerr << "Could not inspect alpha bounds for '" << path.string()
                  << "': " << SDL_GetError() << "\n";
        SDL_DestroySurface(source);
        return false;
    }
    int minimumX = pixels->w;
    int minimumY = pixels->h;
    int maximumX = -1;
    int maximumY = -1;
    const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(pixels->format);
    if (details == nullptr) {
        std::cerr << "Could not inspect pixel format for '" << path.string()
                  << "': " << SDL_GetError() << "\n";
        SDL_DestroySurface(pixels);
        SDL_DestroySurface(source);
        return false;
    }
    const auto* bytes = static_cast<const Uint8*>(pixels->pixels);
    for (int y = 0; y < pixels->h; ++y) {
        const auto* row = reinterpret_cast<const Uint32*>(
            bytes + static_cast<std::size_t>(y) * static_cast<std::size_t>(pixels->pitch));
        for (int x = 0; x < pixels->w; ++x) {
            Uint8 alpha = 0;
            SDL_GetRGBA(row[x], details, nullptr, nullptr, nullptr, nullptr, &alpha);
            if (alpha < 128) {
                continue;
            }
            minimumX = std::min(minimumX, x);
            minimumY = std::min(minimumY, y);
            maximumX = std::max(maximumX, x);
            maximumY = std::max(maximumY, y);
        }
    }
    SDL_DestroySurface(pixels);
    if (maximumX < minimumX || maximumY < minimumY) {
        std::cerr << "Could not load " << (label == nullptr ? "texture" : label)
                  << " '" << path.string()
                  << "': no pixels have alpha >= 128\n";
        SDL_DestroySurface(source);
        return false;
    }

    width_ = static_cast<float>(source->w);
    height_ = static_cast<float>(source->h);
    visibleBounds_ = SDL_Rect{
        minimumX,
        minimumY,
        maximumX - minimumX + 1,
        maximumY - minimumY + 1};
    texture_ = SDL_CreateTextureFromSurface(renderer, source);
    SDL_DestroySurface(source);
    if (texture_ == nullptr) {
        std::cerr << "Could not create " << (label == nullptr ? "texture" : label)
                  << " '" << path.string() << "': " << SDL_GetError() << "\n";
        reset();
        return false;
    }

    if (!SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_NEAREST)) {
        std::cerr << "Could not set nearest-neighbor sampling for '" << path.string()
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
    visibleBounds_.reset();
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

std::optional<SDL_Rect> Texture::visibleBounds() const noexcept {
    return visibleBounds_;
}

}
