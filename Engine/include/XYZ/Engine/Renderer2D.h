#pragma once

#include <string_view>

#include <SDL3/SDL.h>

namespace xyz::engine {

class Texture;

class Renderer2D {
public:
    static constexpr int LogicalWidth = 960;
    static constexpr int LogicalHeight = 540;

    Renderer2D() = default;
    explicit Renderer2D(SDL_Window* window);
    ~Renderer2D();

    Renderer2D(const Renderer2D&) = delete;
    Renderer2D& operator=(const Renderer2D&) = delete;
    Renderer2D(Renderer2D&& other) noexcept;
    Renderer2D& operator=(Renderer2D&& other) noexcept;

    bool initialize(SDL_Window* window);
    void reset() noexcept;

    void clear() const;
    void present() const;
    bool drawTexture(const Texture& texture, const SDL_FRect& destination, bool flipHorizontal = false) const;
    bool drawTexture(
        const Texture& texture,
        const SDL_FRect& destination,
        SDL_Color modulation,
        bool flipHorizontal = false) const;
    bool drawTexture(
        const Texture& texture,
        const SDL_FRect& destination,
        float rotationDegrees,
        const SDL_FPoint& center,
        bool flipHorizontal = false) const;
    bool drawTexture(
        const Texture& texture,
        const SDL_FRect& destination,
        float rotationDegrees,
        const SDL_FPoint& center,
        SDL_Color modulation,
        bool flipHorizontal = false) const;
    bool drawDebugLine(SDL_FPoint start, SDL_FPoint end, SDL_Color color) const;
    bool drawDebugRect(const SDL_FRect& rectangle, SDL_Color color) const;
    void drawDebugText(float x, float y, std::string_view text) const;

    [[nodiscard]] SDL_FPoint windowToLogical(SDL_FPoint windowPoint) const;
    [[nodiscard]] SDL_Renderer* native() const noexcept;
    [[nodiscard]] bool initialized() const noexcept;

private:
    SDL_Renderer* renderer_ = nullptr;
};

}
