#ifndef GLRENDERER_VERTEX_H
#define GLRENDERER_VERTEX_H

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 texCoord{};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f};
};

#endif //GLRENDERER_VERTEX_H
