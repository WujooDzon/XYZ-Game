#pragma once

#include <filesystem>

#include <SDL3/SDL.h>

namespace xyz::engine {

class Texture {
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool load(SDL_Renderer* renderer, const std::filesystem::path& path, const char* label = nullptr);
    void reset() noexcept;

    [[nodiscard]] bool loaded() const noexcept;
    [[nodiscard]] SDL_Texture* native() const noexcept;
    [[nodiscard]] float width() const noexcept;
    [[nodiscard]] float height() const noexcept;

private:
    SDL_Texture* texture_ = nullptr;
    float width_ = 0.0F;
    float height_ = 0.0F;
};

}
