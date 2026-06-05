

add_subdirectory(glad)

add_subdirectory(sdl3)

add_subdirectory(glm-master)

add_subdirectory(imgui-master)

add_subdirectory(tinyobjloader-release)

set(FASTGLTF_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(fastgltf-main)
