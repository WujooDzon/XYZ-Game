#pragma once

#include <string>

namespace xyz::engine {

class Input;
class Renderer2D;

class Scene {
public:
    virtual ~Scene() = default;

    virtual bool initialize(Renderer2D& renderer, std::string& error) = 0;
    virtual void update(float deltaSeconds, const Input& input, Renderer2D& renderer) = 0;
    virtual void render(Renderer2D& renderer) const = 0;
};

}
