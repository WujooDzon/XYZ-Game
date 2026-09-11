#include "XYZ/Engine/Renderer2D.h"

#include <iostream>
#include <string>
#include <utility>

#include "XYZ/Engine/Texture.h"

namespace xyz::engine {

Renderer2D::Renderer2D(SDL_Window* window) {
    initialize(window);
}

Renderer2D::~Renderer2D() {
    reset();
}

Renderer2D::Renderer2D(Renderer2D&& other) noexcept
    : renderer_(other.renderer_) {
    other.renderer_ = nullptr;
}

Renderer2D& Renderer2D::operator=(Renderer2D&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    reset();
    renderer_ = other.renderer_;
    other.renderer_ = nullptr;
    return *this;
}

bool Renderer2D::initialize(SDL_Window* window) {
    reset();
    if (window == nullptr) {
        std::cerr << "Could not create renderer: window is null.\n";
        return false;
    }

    renderer_ = SDL_CreateRenderer(window, nullptr);
    if (renderer_ == nullptr) {
        std::cerr << "Could not create SDL renderer: " << SDL_GetError() << "\n";
        return false;
    }

    if (!SDL_SetRenderLogicalPresentation(
            renderer_, LogicalWidth, LogicalHeight, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) {
        std::cerr << "Could not configure logical presentation: " << SDL_GetError() << "\n";
        reset();
        return false;
    }

    return true;
}

void Renderer2D::reset() noexcept {
    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
}

void Renderer2D::clear() const {
    if (renderer_ == nullptr) {
        return;
    }

    SDL_SetRenderDrawColor(renderer_, 8, 7, 10, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer_);
}

void Renderer2D::present() const {
    if (renderer_ != nullptr) {
        SDL_RenderPresent(renderer_);
    }
}

bool Renderer2D::drawTexture(const Texture& texture, const SDL_FRect& destination, bool flipHorizontal) const {
    return drawTexture(
        texture,
        destination,
        0.0F,
        {destination.w / 2.0F, destination.h / 2.0F},
        {255, 255, 255, SDL_ALPHA_OPAQUE},
        flipHorizontal);
}

bool Renderer2D::drawTexture(
    const Texture& texture,
    const SDL_FRect& destination,
    SDL_Color modulation,
    bool flipHorizontal) const {
    return drawTexture(
        texture,
        destination,
        0.0F,
        {destination.w / 2.0F, destination.h / 2.0F},
        modulation,
        flipHorizontal);
}

bool Renderer2D::drawTexture(
    const Texture& texture,
    const SDL_FRect& destination,
    float rotationDegrees,
    const SDL_FPoint& center,
    bool flipHorizontal) const {
    return drawTexture(
        texture,
        destination,
        rotationDegrees,
        center,
        {255, 255, 255, SDL_ALPHA_OPAQUE},
        flipHorizontal);
}

bool Renderer2D::drawTexture(
    const Texture& texture,
    const SDL_FRect& destination,
    float rotationDegrees,
    const SDL_FPoint& center,
    SDL_Color modulation,
    bool flipHorizontal) const {
    if (renderer_ == nullptr || texture.native() == nullptr) {
        return false;
    }

    Uint8 previousR = 255;
    Uint8 previousG = 255;
    Uint8 previousB = 255;
    Uint8 previousA = SDL_ALPHA_OPAQUE;
    if (!SDL_GetTextureColorMod(texture.native(), &previousR, &previousG, &previousB)
        || !SDL_GetTextureAlphaMod(texture.native(), &previousA)
        || !SDL_SetTextureColorMod(texture.native(), modulation.r, modulation.g, modulation.b)
        || !SDL_SetTextureAlphaMod(texture.native(), modulation.a)) {
        return false;
    }

    const bool drawn = SDL_RenderTextureRotated(
        renderer_,
        texture.native(),
        nullptr,
        &destination,
        static_cast<double>(rotationDegrees),
        &center,
        flipHorizontal ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    const bool restored = SDL_SetTextureColorMod(
                              texture.native(), previousR, previousG, previousB)
        && SDL_SetTextureAlphaMod(texture.native(), previousA);
    return drawn && restored;
}

bool Renderer2D::drawDebugLine(SDL_FPoint start, SDL_FPoint end, SDL_Color color) const {
    if (renderer_ == nullptr || !SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a)) {
        return false;
    }
    return SDL_RenderLine(renderer_, start.x, start.y, end.x, end.y);
}

bool Renderer2D::drawDebugRect(const SDL_FRect& rectangle, SDL_Color color) const {
    if (renderer_ == nullptr || !SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a)) {
        return false;
    }
    return SDL_RenderRect(renderer_, &rectangle);
}

void Renderer2D::drawDebugText(float x, float y, std::string_view text) const {
    if (renderer_ == nullptr) {
        return;
    }

    const std::string textCopy(text);
    SDL_SetRenderDrawColor(renderer_, 255, 245, 220, SDL_ALPHA_OPAQUE);
    SDL_RenderDebugText(renderer_, x, y, textCopy.c_str());
}

SDL_FPoint Renderer2D::windowToLogical(SDL_FPoint windowPoint) const {
    SDL_FPoint logicalPoint{windowPoint.x, windowPoint.y};
    if (renderer_ == nullptr) {
        return logicalPoint;
    }

    SDL_RenderCoordinatesFromWindow(
        renderer_, windowPoint.x, windowPoint.y, &logicalPoint.x, &logicalPoint.y);
    return logicalPoint;
}

SDL_Renderer* Renderer2D::native() const noexcept {
    return renderer_;
}

bool Renderer2D::initialized() const noexcept {
    return renderer_ != nullptr;
}

}
