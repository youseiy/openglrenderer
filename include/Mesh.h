#ifndef GLRENDERER_MESH_H
#define GLRENDERER_MESH_H

#include <cstddef>
#include <vector>

#include "RenderModel.h"

class ShaderProgram;

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void Create(
        const Vertex* vertices,
        std::size_t vertexCount,
        const unsigned int* indices,
        std::size_t indexCount
    );
    void Create(RenderModel&& model);

    void Draw(
        const ShaderProgram& shader,
        float fallbackMetallic,
        float fallbackRoughness,
        float fallbackAmbientOcclusion,
        float opacity
    ) const;
    void Destroy();

private:
    void UploadGeometry(
        const Vertex* vertices,
        std::size_t vertexCount,
        const unsigned int* indices,
        std::size_t indexCount
    );

    unsigned int m_vao{};
    unsigned int m_vbo{};
    unsigned int m_ebo{};
    std::size_t m_indexCount{};
    std::vector<RenderMaterial> m_materials;
    std::vector<RenderPrimitive> m_primitives;
};

#endif //GLRENDERER_MESH_H
