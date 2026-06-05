#ifndef GLRENDERER_RENDERMODEL_H
#define GLRENDERER_RENDERMODEL_H

#include "Vertex.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

enum class MaterialAlphaMode {
    Opaque,
    Mask,
    Blend
};

struct RenderMaterial {
    glm::vec4 baseColorFactor{1.0f};
    glm::vec3 emissiveFactor{};
    float metallicFactor{1.0f};
    float roughnessFactor{1.0f};
    float occlusionStrength{1.0f};
    float normalScale{1.0f};
    float alphaCutoff{0.5f};
    MaterialAlphaMode alphaMode{MaterialAlphaMode::Opaque};
    bool doubleSided{};
    bool useFallbackPbr{};

    unsigned int baseColorTexture{};
    unsigned int metallicRoughnessTexture{};
    unsigned int normalTexture{};
    unsigned int occlusionTexture{};
    unsigned int emissiveTexture{};
};

struct RenderPrimitive {
    std::size_t firstIndex{};
    std::size_t indexCount{};
    std::size_t materialIndex{};
};

struct RenderModel {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<RenderMaterial> materials;
    std::vector<RenderPrimitive> primitives;
};

#endif //GLRENDERER_RENDERMODEL_H
