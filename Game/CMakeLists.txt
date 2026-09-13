add_executable(XYZGame
    src/main.cpp
    src/GameApp.cpp
    src/GuffmanBasementScene.cpp
    src/FootPlantController.cpp
    src/PlayerController.cpp
    src/RigReviewExporter.cpp
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
    src/FootPlantController.cpp
    src/GuffmanBasementScene.cpp
    src/PlayerController.cpp
    src/RigReviewExporter.cpp
)

target_include_directories(XYZGuffmanBasementSceneTests PRIVATE include)
target_link_libraries(XYZGuffmanBasementSceneTests PRIVATE XYZEngine)
add_test(NAME XYZGuffmanBasementSceneTests COMMAND XYZGuffmanBasementSceneTests)
set_tests_properties(XYZGuffmanBasementSceneTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(XYZFootPlantControllerTests
    Tests/FootPlantControllerTests.cpp
    src/FootPlantController.cpp
)

target_include_directories(XYZFootPlantControllerTests PRIVATE include)
target_link_libraries(XYZFootPlantControllerTests PRIVATE XYZEngine)
add_test(NAME XYZFootPlantControllerTests COMMAND XYZFootPlantControllerTests)

add_executable(XYZLogenRigV3AssetTests
    Tests/LogenRigV3AssetTests.cpp
)

target_link_libraries(XYZLogenRigV3AssetTests PRIVATE XYZEngine)
add_test(NAME XYZLogenRigV3AssetTests COMMAND XYZLogenRigV3AssetTests)
set_tests_properties(XYZLogenRigV3AssetTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(XYZRigReviewExporterTests
    Tests/RigReviewExporterTests.cpp
    src/RigReviewExporter.cpp
)

target_include_directories(XYZRigReviewExporterTests PRIVATE include)
target_link_libraries(XYZRigReviewExporterTests PRIVATE XYZEngine)
add_test(NAME XYZRigReviewExporterTests COMMAND XYZRigReviewExporterTests)
set_tests_properties(XYZRigReviewExporterTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(XYZLogenFrameAnimationAssetTests
    Tests/LogenFrameAnimationAssetTests.cpp
)

target_link_libraries(XYZLogenFrameAnimationAssetTests PRIVATE XYZEngine)
add_test(NAME XYZLogenFrameAnimationAssetTests COMMAND XYZLogenFrameAnimationAssetTests)
set_tests_properties(XYZLogenFrameAnimationAssetTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
