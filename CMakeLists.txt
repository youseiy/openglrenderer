cmake_minimum_required(VERSION 3.20)

project(glrenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



add_subdirectory(dependencies)

add_executable(glrenderer
    src/main.cpp
    src/Shader.cpp
    src/ShaderProgram.cpp
    src/Buffer.cpp
    src/OpenGLRenderer.cpp
    src/Camera.cpp
    src/OrbitCameraController.cpp
    src/Mesh.cpp
    src/ImGuiLayer.cpp
    src/ObjLoader.cpp
    src/GltfLoader.cpp
)

target_include_directories(glrenderer PUBLIC
    include
)

target_include_directories(glrenderer PRIVATE
    dependencies/tinyobjloader-release/examples/viewer
)

target_link_libraries(glrenderer PRIVATE
    glad
    SDL3::SDL3
    glm-header-only
    imgui
    tinyobjloader
    fastgltf::fastgltf
)

add_custom_command(TARGET glrenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:glrenderer>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:glrenderer>/assets
)
