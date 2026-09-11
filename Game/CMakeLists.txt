add_executable(XYZGame
    src/main.cpp
    src/GameApp.cpp
    src/GuffmanBasementScene.cpp
    src/PlayerController.cpp
)

target_include_directories(XYZGame PRIVATE include)
target_link_libraries(XYZGame PRIVATE XYZEngine)

add_executable(XYZPlayerControllerTests
    Tests/PlayerControllerTests.cpp
    src/PlayerController.cpp
)

target_include_directories(XYZPlayerControllerTests PRIVATE include)
target_link_libraries(XYZPlayerControllerTests PRIVATE XYZEngine)
add_test(NAME XYZPlayerControllerTests COMMAND XYZPlayerControllerTests)

add_executable(XYZGuffmanBasementSceneTests
    Tests/GuffmanBasementSceneTests.cpp
    src/GuffmanBasementScene.cpp
    src/PlayerController.cpp
)

target_include_directories(XYZGuffmanBasementSceneTests PRIVATE include)
target_link_libraries(XYZGuffmanBasementSceneTests PRIVATE XYZEngine)
add_test(NAME XYZGuffmanBasementSceneTests COMMAND XYZGuffmanBasementSceneTests)
set_tests_properties(XYZGuffmanBasementSceneTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
