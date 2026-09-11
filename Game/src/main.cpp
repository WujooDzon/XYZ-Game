#include "XYZ/Game/GameApp.h"

int main(int argc, char** argv) {
    const auto projectRoot = xyz::game::GameApp::discoverProjectRoot(argc > 0 ? argv[0] : nullptr);
    xyz::game::GameApp application(projectRoot);
    return application.run(argc, argv);
}
