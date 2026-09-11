#pragma once

#include <filesystem>

namespace xyz::game {

class GameApp {
public:
    explicit GameApp(std::filesystem::path projectRoot);

    [[nodiscard]] int run(int argc, char** argv) const;

    [[nodiscard]] static std::filesystem::path discoverProjectRoot(const char* executablePath);

private:
    [[nodiscard]] int runSelfTest() const;

    std::filesystem::path projectRoot_;
};

}
