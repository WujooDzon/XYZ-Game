#pragma once

#include <string>

namespace xyz::engine {

class Scene;

class Application {
public:
    explicit Application(std::string title = "XYZ Game");

    [[nodiscard]] int run(Scene& scene) const;

private:
    std::string title_;
};

}
