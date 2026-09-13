#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

namespace {

struct AlphaBounds {
    int minX;
    int minY;
    int maxX;
    int maxY;
};

AlphaBounds alphaBounds(SDL_Surface* surface) {
    const auto* details = SDL_GetPixelFormatDetails(surface->format);
    const auto* bytes = static_cast<const Uint8*>(surface->pixels);
    AlphaBounds bounds{surface->w, surface->h, -1, -1};
    for (int y = 0; y < surface->h; ++y) {
        const auto* row = reinterpret_cast<const Uint32*>(bytes + y * surface->pitch);
        for (int x = 0; x < surface->w; ++x) {
            Uint8 alpha = 0;
            SDL_GetRGBA(row[x], details, nullptr, nullptr, nullptr, nullptr, &alpha);
            if (alpha < 128) {
                continue;
            }
            bounds.minX = std::min(bounds.minX, x);
            bounds.minY = std::min(bounds.minY, y);
            bounds.maxX = std::max(bounds.maxX, x);
            bounds.maxY = std::max(bounds.maxY, y);
        }
    }
    return bounds;
}

} // namespace

int main() {
    const auto check = [](bool condition, const std::string& message) {
        if (!condition) {
            std::cerr << "Logen frame animation asset test failed: " << message << "\n";
            std::exit(1);
        }
    };

    const std::filesystem::path directory =
        "Assets/Characters/Logen/WalkV2";
    const std::array<const char*, 8> names{
        "Logen_walk_right_01.png",
        "Logen_walk_right_02.png",
        "Logen_walk_right_03.png",
        "Logen_walk_right_04.png",
        "Logen_walk_right_05.png",
        "Logen_walk_right_06.png",
        "Logen_walk_right_07.png",
        "Logen_walk_right_08.png"};

    int minimumBaseline = 512;
    int maximumBaseline = -1;
    int minimumVisibleHeight = 512;
    int maximumVisibleHeight = 0;
    for (const char* name : names) {
        const auto path = directory / name;
        SDL_Surface* source = IMG_Load(path.string().c_str());
        check(source != nullptr, "missing generated walk frame " + path.string());
        SDL_Surface* rgba = SDL_ConvertSurface(source, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(source);
        check(rgba != nullptr, "could not convert " + path.string() + " to RGBA");
        check(rgba->w == 384 && rgba->h == 512,
              std::string(name) + " does not use the shared 384x512 canvas");

        const AlphaBounds bounds = alphaBounds(rgba);
        check(bounds.maxX >= bounds.minX && bounds.maxY >= bounds.minY,
              std::string(name) + " has no visible sprite pixels");
        check(bounds.maxY - bounds.minY > 350,
              std::string(name) + " does not contain a complete full-body sprite");

        const auto* details = SDL_GetPixelFormatDetails(rgba->format);
        const auto* row = reinterpret_cast<const Uint32*>(rgba->pixels);
        Uint8 cornerAlpha = 255;
        SDL_GetRGBA(row[0], details, nullptr, nullptr, nullptr, nullptr, &cornerAlpha);
        check(cornerAlpha == 0,
              std::string(name) + " has a baked background instead of alpha transparency");

        minimumBaseline = std::min(minimumBaseline, bounds.maxY);
        maximumBaseline = std::max(maximumBaseline, bounds.maxY);
        const int visibleHeight = bounds.maxY - bounds.minY + 1;
        minimumVisibleHeight = std::min(minimumVisibleHeight, visibleHeight);
        maximumVisibleHeight = std::max(maximumVisibleHeight, visibleHeight);
        std::cout << name << " baseline=" << bounds.maxY
                  << " visibleHeight=" << visibleHeight << "\n";
        SDL_DestroySurface(rgba);
    }

    check(maximumBaseline - minimumBaseline <= 6,
          "walk frames do not share a stable foot baseline");
    check(maximumVisibleHeight - minimumVisibleHeight <= 48,
          "walk cycle changes character height too much between poses");
    std::cout << "Logen frame animation asset tests passed.\n";
}
