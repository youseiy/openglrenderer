#include "GltfLoader.h"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <glad/glad.h>
#include <glm/gtc/matrix_inverse.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <vector>

namespace {
glm::mat4 ToGlm(const fastgltf::math::fmat4x4& matrix) {
    // Both libraries use column-major matrices, so only an element-wise copy is needed.
    glm::mat4 result{1.0f};

    for (std::size_t column = 0; column < 4; ++column)
    {
        for (std::size_t row = 0; row < 4; ++row)
        {
            result[column][row] = matrix[column][row];
        }
    }

    return result;
}

unsigned int CreateTexture(const unsigned char* bytes, const int byteCount) {
    int width{};
    int height{};
    int sourceChannels{};
    unsigned char* pixels = stbi_load_from_memory(
        bytes,
        byteCount,
        &width,
        &height,
        &sourceChannels,
        STBI_rgb_alpha
    );

    if (!pixels)
    {
        std::cout << "glTF image decode error: " << stbi_failure_reason() << '\n';
        return 0;
    }

    unsigned int texture{};
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(pixels);
    return texture;
}

unsigned int LoadImageTexture(
    const fastgltf::Asset& asset,
    const std::filesystem::path& modelDirectory,
    const std::size_t textureIndex
) {
    const fastgltf::Texture& texture = asset.textures[textureIndex];
    if (!texture.imageIndex.has_value())
    {
        return 0;
    }

    const fastgltf::Image& image = asset.images[*texture.imageIndex];
    // Images can live beside the glTF file, inside a GLB buffer view, or in memory.
    return std::visit(
        fastgltf::visitor{
            [](const auto&) -> unsigned int {
                return 0;
            },
            [&](const fastgltf::sources::URI& source) -> unsigned int {
                if (!source.uri.isLocalPath())
                {
                    return 0;
                }

                const std::string relativePath{
                    source.uri.path().begin(),
                    source.uri.path().end()
                };
                int width{};
                int height{};
                int channels{};
                unsigned char* pixels = stbi_load(
                    (modelDirectory / relativePath).string().c_str(),
                    &width,
                    &height,
                    &channels,
                    STBI_rgb_alpha
                );

                if (!pixels)
                {
                    return 0;
                }

                unsigned int id{};
                glGenTextures(1, &id);
                glBindTexture(GL_TEXTURE_2D, id);
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGBA8,
                    width,
                    height,
                    0,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    pixels
                );
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                stbi_image_free(pixels);
                return id;
            },
            [&](const fastgltf::sources::Array& source) -> unsigned int {
                return CreateTexture(
                    reinterpret_cast<const unsigned char*>(source.bytes.data()),
                    static_cast<int>(source.bytes.size())
                );
            },
            [&](const fastgltf::sources::ByteView& source) -> unsigned int {
                return CreateTexture(
                    reinterpret_cast<const unsigned char*>(source.bytes.data()),
                    static_cast<int>(source.bytes.size())
                );
            },
            [&](const fastgltf::sources::BufferView& source) -> unsigned int {
                const fastgltf::BufferView& view = asset.bufferViews[source.bufferViewIndex];
                const fastgltf::Buffer& buffer = asset.buffers[view.bufferIndex];

                return std::visit(
                    fastgltf::visitor{
                        [](const auto&) -> unsigned int {
                            return 0;
                        },
                        [&](const fastgltf::sources::Array& data) -> unsigned int {
                            return CreateTexture(
                                reinterpret_cast<const unsigned char*>(
                                    data.bytes.data() + view.byteOffset
                                ),
                                static_cast<int>(view.byteLength)
                            );
                        },
                        [&](const fastgltf::sources::ByteView& data) -> unsigned int {
                            return CreateTexture(
                                reinterpret_cast<const unsigned char*>(
                                    data.bytes.data() + view.byteOffset
                                ),
                                static_cast<int>(view.byteLength)
                            );
                        }
                    },
                    buffer.data
                );
            }
        },
        image.data
    );
}

RenderMaterial LoadMaterial(
    const fastgltf::Asset& asset,
    const fastgltf::Material& source,
    const std::filesystem::path& modelDirectory
) {
    // glTF stores material factors separately from textures; the shader multiplies both.
    RenderMaterial material;
    const auto& baseColor = source.pbrData.baseColorFactor;
    const auto& emissive = source.emissiveFactor;

    material.baseColorFactor = {baseColor[0], baseColor[1], baseColor[2], baseColor[3]};
    material.emissiveFactor = glm::vec3{
        emissive[0],
        emissive[1],
        emissive[2]
    } * static_cast<float>(source.emissiveStrength);
    material.metallicFactor = source.pbrData.metallicFactor;
    material.roughnessFactor = source.pbrData.roughnessFactor;
    material.alphaCutoff = source.alphaCutoff;
    material.doubleSided = source.doubleSided;

    switch (source.alphaMode)
    {
        case fastgltf::AlphaMode::Mask:
            material.alphaMode = MaterialAlphaMode::Mask;
            break;
        case fastgltf::AlphaMode::Blend:
            material.alphaMode = MaterialAlphaMode::Blend;
            break;
        default:
            material.alphaMode = MaterialAlphaMode::Opaque;
            break;
    }

    if (source.pbrData.baseColorTexture)
    {
        material.baseColorTexture = LoadImageTexture(
            asset,
            modelDirectory,
            source.pbrData.baseColorTexture->textureIndex
        );
    }

    if (source.pbrData.metallicRoughnessTexture)
    {
        material.metallicRoughnessTexture = LoadImageTexture(
            asset,
            modelDirectory,
            source.pbrData.metallicRoughnessTexture->textureIndex
        );
    }

    if (source.normalTexture)
    {
        material.normalTexture = LoadImageTexture(
            asset,
            modelDirectory,
            source.normalTexture->textureIndex
        );
        material.normalScale = source.normalTexture->scale;
    }

    if (source.occlusionTexture)
    {
        material.occlusionTexture = LoadImageTexture(
            asset,
            modelDirectory,
            source.occlusionTexture->textureIndex
        );
        material.occlusionStrength = source.occlusionTexture->strength;
    }

    if (source.emissiveTexture)
    {
        material.emissiveTexture = LoadImageTexture(
            asset,
            modelDirectory,
            source.emissiveTexture->textureIndex
        );
    }

    return material;
}

void GenerateNormals(
    std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
    const std::size_t firstVertex,
    const std::size_t firstIndex
) {
    // Accumulating unnormalized face normals produces area-weighted smooth normals.
    for (std::size_t index = firstIndex; index + 2 < indices.size(); index += 3)
    {
        Vertex& a = vertices[indices[index]];
        Vertex& b = vertices[indices[index + 1]];
        Vertex& c = vertices[indices[index + 2]];
        const glm::vec3 normal = glm::cross(
            b.position - a.position,
            c.position - a.position
        );

        if (glm::dot(normal, normal) > std::numeric_limits<float>::epsilon())
        {
            a.normal += normal;
            b.normal += normal;
            c.normal += normal;
        }
    }

    for (std::size_t index = firstVertex; index < vertices.size(); ++index)
    {
        const float lengthSquared = glm::dot(vertices[index].normal, vertices[index].normal);
        vertices[index].normal = lengthSquared > std::numeric_limits<float>::epsilon()
            ? glm::normalize(vertices[index].normal)
            : glm::vec3{0.0f, 1.0f, 0.0f};
    }
}

void GenerateTangents(
    std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
    const std::size_t firstVertex,
    const std::size_t firstIndex
) {
    // Tangent space is required to transform normal maps into world space.
    std::vector<glm::vec3> tangentSums(vertices.size() - firstVertex);
    std::vector<glm::vec3> bitangentSums(vertices.size() - firstVertex);

    for (std::size_t index = firstIndex; index + 2 < indices.size(); index += 3)
    {
        const unsigned int ia = indices[index];
        const unsigned int ib = indices[index + 1];
        const unsigned int ic = indices[index + 2];
        const Vertex& a = vertices[ia];
        const Vertex& b = vertices[ib];
        const Vertex& c = vertices[ic];
        const glm::vec3 edge1 = b.position - a.position;
        const glm::vec3 edge2 = c.position - a.position;
        const glm::vec2 uv1 = b.texCoord - a.texCoord;
        const glm::vec2 uv2 = c.texCoord - a.texCoord;
        const float determinant = uv1.x * uv2.y - uv1.y * uv2.x;

        if (std::abs(determinant) <= std::numeric_limits<float>::epsilon())
        {
            // Degenerate UVs have no well-defined tangent basis.
            continue;
        }

        const float inverse = 1.0f / determinant;
        const glm::vec3 tangent = (edge1 * uv2.y - edge2 * uv1.y) * inverse;
        const glm::vec3 bitangent = (edge2 * uv1.x - edge1 * uv2.x) * inverse;

        for (const unsigned int vertexIndex : {ia, ib, ic})
        {
            tangentSums[vertexIndex - firstVertex] += tangent;
            bitangentSums[vertexIndex - firstVertex] += bitangent;
        }
    }

    for (std::size_t index = firstVertex; index < vertices.size(); ++index)
    {
        const glm::vec3 normal = vertices[index].normal;
        glm::vec3 tangent = tangentSums[index - firstVertex];

        if (glm::dot(tangent, tangent) <= std::numeric_limits<float>::epsilon())
        {
            // Build a stable basis when the mesh has no useful UV gradient.
            const glm::vec3 axis = std::abs(normal.y) < 0.999f
                ? glm::vec3{0.0f, 1.0f, 0.0f}
                : glm::vec3{1.0f, 0.0f, 0.0f};
            tangent = glm::normalize(glm::cross(axis, normal));
        }
        else
        {
            tangent = glm::normalize(tangent - normal * glm::dot(normal, tangent));
        }

        const float handedness =
            glm::dot(glm::cross(normal, tangent), bitangentSums[index - firstVertex]) < 0.0f
                ? -1.0f
                : 1.0f;
        vertices[index].tangent = glm::vec4(tangent, handedness);
    }
}

bool AppendPrimitive(
    const fastgltf::Asset& asset,
    const fastgltf::Primitive& primitive,
    const glm::mat4& transform,
    RenderModel& model
) {
    // The renderer currently submits indexed triangle lists only.
    if (primitive.type != fastgltf::PrimitiveType::Triangles)
    {
        return true;
    }

    const auto* positionAttribute = primitive.findAttribute("POSITION");
    if (positionAttribute == primitive.attributes.end() || !primitive.indicesAccessor)
    {
        return false;
    }

    const auto& positionAccessor = asset.accessors[positionAttribute->accessorIndex];
    const std::size_t firstVertex = model.vertices.size();
    const std::size_t firstIndex = model.indices.size();
    model.vertices.resize(firstVertex + positionAccessor.count);

    // Node transforms are baked into vertices so the current renderer can use one model matrix.
    fastgltf::iterateAccessorWithIndex<glm::vec3>(
        asset,
        positionAccessor,
        [&](const glm::vec3& position, const std::size_t index) {
            model.vertices[firstVertex + index].position =
                glm::vec3(transform * glm::vec4(position, 1.0f));
        }
    );

    if (const auto* colorAttribute = primitive.findAttribute("COLOR_0");
        colorAttribute != primitive.attributes.end())
    {
        const auto& colorAccessor = asset.accessors[colorAttribute->accessorIndex];
        if (colorAccessor.type == fastgltf::AccessorType::Vec3)
        {
            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                asset,
                colorAccessor,
                [&](const glm::vec3& color, const std::size_t index) {
                    model.vertices[firstVertex + index].color = color;
                }
            );
        }
        else if (colorAccessor.type == fastgltf::AccessorType::Vec4)
        {
            fastgltf::iterateAccessorWithIndex<glm::vec4>(
                asset,
                colorAccessor,
                [&](const glm::vec4& color, const std::size_t index) {
                    model.vertices[firstVertex + index].color = glm::vec3(color);
                }
            );
        }
    }

    if (const auto* texCoordAttribute = primitive.findAttribute("TEXCOORD_0");
        texCoordAttribute != primitive.attributes.end())
    {
        fastgltf::iterateAccessorWithIndex<glm::vec2>(
            asset,
            asset.accessors[texCoordAttribute->accessorIndex],
            [&](const glm::vec2& texCoord, const std::size_t index) {
                model.vertices[firstVertex + index].texCoord = texCoord;
            }
        );
    }

    bool hasNormals = false;
    if (const auto* normalAttribute = primitive.findAttribute("NORMAL");
        normalAttribute != primitive.attributes.end())
    {
        const glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(transform));
        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            asset,
            asset.accessors[normalAttribute->accessorIndex],
            [&](const glm::vec3& normal, const std::size_t index) {
                model.vertices[firstVertex + index].normal =
                    glm::normalize(normalMatrix * normal);
            }
        );
        hasNormals = true;
    }

    const auto& indexAccessor = asset.accessors[*primitive.indicesAccessor];
    std::vector<std::uint32_t> primitiveIndices(indexAccessor.count);
    fastgltf::copyFromAccessor<std::uint32_t>(
        asset,
        indexAccessor,
        primitiveIndices.data()
    );

    model.indices.reserve(model.indices.size() + primitiveIndices.size());
    for (const std::uint32_t index : primitiveIndices)
    {
        model.indices.push_back(static_cast<unsigned int>(firstVertex + index));
    }

    if (!hasNormals)
    {
        GenerateNormals(model.vertices, model.indices, firstVertex, firstIndex);
    }

    bool hasTangents = false;
    if (const auto* tangentAttribute = primitive.findAttribute("TANGENT");
        tangentAttribute != primitive.attributes.end())
    {
        const glm::mat3 tangentMatrix{transform};
        fastgltf::iterateAccessorWithIndex<glm::vec4>(
            asset,
            asset.accessors[tangentAttribute->accessorIndex],
            [&](const glm::vec4& tangent, const std::size_t index) {
                model.vertices[firstVertex + index].tangent = {
                    glm::normalize(tangentMatrix * glm::vec3(tangent)),
                    tangent.w
                };
            }
        );
        hasTangents = true;
    }

    if (!hasTangents)
    {
        GenerateTangents(model.vertices, model.indices, firstVertex, firstIndex);
    }

    model.primitives.push_back({
        firstIndex,
        model.indices.size() - firstIndex,
        // Slot zero is the renderer's default material.
        primitive.materialIndex ? *primitive.materialIndex + 1 : 0
    });
    return true;
}
}

bool GltfLoader::Load(const char* filePath, RenderModel& model) {
    const std::filesystem::path path{filePath};
    auto file = fastgltf::MappedGltfFile::FromPath(path);

    if (!file)
    {
        std::cout << "glTF file error: "
                  << fastgltf::getErrorMessage(file.error())
                  << '\n';
        return false;
    }

    fastgltf::Parser parser{
        fastgltf::Extensions::KHR_mesh_quantization |
        fastgltf::Extensions::KHR_texture_transform
    };
    constexpr auto options =
        fastgltf::Options::LoadExternalBuffers |
        fastgltf::Options::LoadExternalImages |
        fastgltf::Options::GenerateMeshIndices;
    auto result = parser.loadGltf(file.get(), path.parent_path(), options);

    if (!result)
    {
        std::cout << "glTF parse error: "
                  << fastgltf::getErrorMessage(result.error())
                  << '\n';
        return false;
    }

    fastgltf::Asset asset = std::move(result.get());
    if (asset.scenes.empty())
    {
        std::cout << "glTF contains no scenes\n";
        return false;
    }

    model = {};
    // Keep a default material for primitives that do not reference one.
    model.materials.emplace_back();
    for (const fastgltf::Material& material : asset.materials)
    {
        model.materials.push_back(LoadMaterial(asset, material, path.parent_path()));
    }

    const std::size_t sceneIndex = asset.defaultScene.value_or(0);
    bool valid = true;
    // Scene traversal resolves the complete parent-child transform hierarchy.
    fastgltf::iterateSceneNodes(
        asset,
        sceneIndex,
        fastgltf::math::fmat4x4{},
        [&](const fastgltf::Node& node, const fastgltf::math::fmat4x4& matrix) {
            if (!valid || !node.meshIndex)
            {
                return;
            }

            const glm::mat4 transform = ToGlm(matrix);
            for (const fastgltf::Primitive& primitive : asset.meshes[*node.meshIndex].primitives)
            {
                valid = AppendPrimitive(asset, primitive, transform, model);
                if (!valid)
                {
                    break;
                }
            }
        }
    );

    return valid && !model.vertices.empty() && !model.indices.empty();
}
