add_library(XYZEngine STATIC
    src/Animation.cpp
    src/Application.cpp
    src/Input.cpp
    src/Json.cpp
    src/Renderer2D.cpp
    src/RigAnimation.cpp
    src/Rig2D.cpp
    src/Sprite.cpp
    src/Texture.cpp
)

target_compile_features(XYZEngine PUBLIC cxx_std_20)
target_include_directories(XYZEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(XYZEngine
    PUBLIC
        SDL3::SDL3
        SDL3_image::SDL3_image
)

add_executable(XYZAnimationTests
    Tests/AnimationTests.cpp
)

target_link_libraries(XYZAnimationTests PRIVATE XYZEngine)
add_test(NAME XYZAnimationTests COMMAND XYZAnimationTests)

add_executable(XYZInputTests
    Tests/InputTests.cpp
)

target_link_libraries(XYZInputTests PRIVATE XYZEngine)
add_test(NAME XYZInputTests COMMAND XYZInputTests)

add_executable(XYZJsonTests
    Tests/JsonTests.cpp
)

target_link_libraries(XYZJsonTests PRIVATE XYZEngine)
add_test(NAME XYZJsonTests COMMAND XYZJsonTests)

add_executable(XYZRendererTests
    Tests/Renderer2DTests.cpp
)

target_link_libraries(XYZRendererTests PRIVATE XYZEngine)
add_test(NAME XYZRendererTests COMMAND XYZRendererTests)

add_executable(XYZRig2DTests
    Tests/Rig2DTests.cpp
)

target_link_libraries(XYZRig2DTests PRIVATE XYZEngine)
add_test(NAME XYZRig2DTests COMMAND XYZRig2DTests)
set_tests_properties(XYZRig2DTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(XYZRigAnimationTests
    Tests/RigAnimationTests.cpp
)

target_link_libraries(XYZRigAnimationTests PRIVATE XYZEngine)
add_test(NAME XYZRigAnimationTests COMMAND XYZRigAnimationTests)
set_tests_properties(XYZRendererTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
