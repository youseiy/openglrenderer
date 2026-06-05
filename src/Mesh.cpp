#include "Mesh.h"

#include "ShaderProgram.h"

#include <glad/glad.h>

#include <cstddef>
#include <utility>

namespace {
void BindTexture(
    const ShaderProgram& shader,
    const char* samplerName,
    const char* enabledName,
    const unsigned int unit,
    const unsigned int texture
) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    shader.SetInt(samplerName, static_cast<int>(unit));
    shader.SetInt(enabledName, texture != 0);
}

void ApplyMaterial(
    const ShaderProgram& shader,
    const RenderMaterial& material,
    const float fallbackMetallic,
    const float fallbackRoughness,
    const float fallbackAmbientOcclusion,
    const float opacity
) {
    // Legacy formats use editor defaults; glTF materials provide their own PBR factors.
    shader.SetVec4("uBaseColorFactor", material.baseColorFactor);
    shader.SetVec3("uEmissiveFactor", material.emissiveFactor);
    shader.SetFloat(
        "uMetallic",
        material.useFallbackPbr ? fallbackMetallic : material.metallicFactor
    );
    shader.SetFloat(
        "uRoughness",
        material.useFallbackPbr ? fallbackRoughness : material.roughnessFactor
    );
    shader.SetFloat(
        "uAmbientOcclusion",
        material.useFallbackPbr ? fallbackAmbientOcclusion : 1.0f
    );
    shader.SetFloat("uOcclusionStrength", material.occlusionStrength);
    shader.SetFloat("uNormalScale", material.normalScale);
    shader.SetFloat("uAlphaCutoff", material.alphaCutoff);
    shader.SetInt("uAlphaMode", static_cast<int>(material.alphaMode));
    shader.SetOpacity(opacity);

    BindTexture(shader, "uBaseColorMap", "uHasBaseColorMap", 0, material.baseColorTexture);
    BindTexture(
        shader,
        "uMetallicRoughnessMap",
        "uHasMetallicRoughnessMap",
        1,
        material.metallicRoughnessTexture
    );
    BindTexture(shader, "uNormalMap", "uHasNormalMap", 2, material.normalTexture);
    BindTexture(shader, "uOcclusionMap", "uHasOcclusionMap", 3, material.occlusionTexture);
    BindTexture(shader, "uEmissiveMap", "uHasEmissiveMap", 4, material.emissiveTexture);

    if (material.doubleSided)
    {
        glDisable(GL_CULL_FACE);
    }
    else
    {
        glEnable(GL_CULL_FACE);
    }
}
}

Mesh::~Mesh() {
    Destroy();
}

void Mesh::Create(
    const Vertex* vertices,
    const std::size_t vertexCount,
    const unsigned int* indices,
    const std::size_t indexCount
) {
    Destroy();

    RenderMaterial material;
    material.useFallbackPbr = true;
    m_materials.push_back(material);
    m_primitives.push_back({0, indexCount, 0});

    UploadGeometry(vertices, vertexCount, indices, indexCount);
}

void Mesh::Create(RenderModel&& model) {
    Destroy();

    m_materials = std::move(model.materials);
    m_primitives = std::move(model.primitives);

    if (m_materials.empty())
    {
        m_materials.emplace_back();
    }

    UploadGeometry(
        model.vertices.data(),
        model.vertices.size(),
        model.indices.data(),
        model.indices.size()
    );
}

void Mesh::UploadGeometry(
    const Vertex* vertices,
    const std::size_t vertexCount,
    const unsigned int* indices,
    const std::size_t indexCount
) {
    m_indexCount = indexCount;

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertexCount * sizeof(Vertex)),
        vertices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indexCount * sizeof(unsigned int)),
        indices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, position))
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, normal))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, texCoord))
    );
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(
        3,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, color))
    );
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(
        4,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, tangent))
    );
    glEnableVertexAttribArray(4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Mesh::Draw(
    const ShaderProgram& shader,
    const float fallbackMetallic,
    const float fallbackRoughness,
    const float fallbackAmbientOcclusion,
    const float opacity
) const {
    glBindVertexArray(m_vao);

    // Opaque geometry writes depth first. Blended geometry tests depth without replacing it.
    const auto drawPass = [&](const bool transparent) {
        glDepthMask(transparent ? GL_FALSE : GL_TRUE);

        for (const RenderPrimitive& primitive : m_primitives)
        {
            const RenderMaterial& material = m_materials[primitive.materialIndex];
            const bool isTransparent = material.alphaMode == MaterialAlphaMode::Blend;

            if (isTransparent != transparent)
            {
                continue;
            }

            ApplyMaterial(
                shader,
                material,
                fallbackMetallic,
                fallbackRoughness,
                fallbackAmbientOcclusion,
                opacity
            );
            glDrawElements(
                GL_TRIANGLES,
                static_cast<int>(primitive.indexCount),
                GL_UNSIGNED_INT,
                reinterpret_cast<const void*>(primitive.firstIndex * sizeof(unsigned int))
            );
        }
    };

    drawPass(false);
    drawPass(true);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

void Mesh::Destroy() {
    // Texture ownership is transferred from RenderModel to Mesh during creation.
    for (const RenderMaterial& material : m_materials)
    {
        const unsigned int textures[] = {
            material.baseColorTexture,
            material.metallicRoughnessTexture,
            material.normalTexture,
            material.occlusionTexture,
            material.emissiveTexture
        };

        for (const unsigned int texture : textures)
        {
            if (texture != 0)
            {
                glDeleteTextures(1, &texture);
            }
        }
    }

    m_materials.clear();
    m_primitives.clear();

    if (m_vao != 0)
    {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    if (m_vbo != 0)
    {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_ebo != 0)
    {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }

    m_indexCount = 0;
}
