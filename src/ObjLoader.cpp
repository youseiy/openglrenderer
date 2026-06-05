#include "ObjLoader.h"

#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace {
std::string GetParentDirectory(const char* filePath) {
    const std::string path{filePath};
    const std::size_t slash = path.find_last_of("/\\");

    if (slash == std::string::npos)
    {
        return {};
    }

    return path.substr(0, slash + 1);
}

bool IsDefaultGray(const glm::vec3& color) {
    constexpr float DefaultGray = 0.64f;
    constexpr float Epsilon = 0.001f;

    return std::abs(color.r - DefaultGray) < Epsilon &&
           std::abs(color.g - DefaultGray) < Epsilon &&
           std::abs(color.b - DefaultGray) < Epsilon;
}

glm::vec3 GetDisplayColor(const tinyobj::material_t& material) {
    glm::vec3 color{
        material.diffuse[0],
        material.diffuse[1],
        material.diffuse[2]
    };

    if (!IsDefaultGray(color))
    {
        return color;
    }

    if (material.name == "ground")
    {
        return {0.23f, 0.42f, 0.20f};
    }

    if (material.name == "light_1")
    {
        return {1.0f, 0.82f, 0.38f};
    }

    if (material.name == "cottage_texture")
    {
        return {0.68f, 0.47f, 0.30f};
    }

    return color;
}
}

bool ObjLoader::Load(
    const char* filePath,
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices
) {
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    config.mtl_search_path = GetParentDirectory(filePath);

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filePath, config))
    {
        if (!reader.Error().empty())
        {
            std::cout << "OBJ load error: "
                      << reader.Error()
                      << '\n';
        }

        return false;
    }

    if (!reader.Warning().empty())
    {
        std::cout << "OBJ load warning: "
                  << reader.Warning()
                  << '\n';
    }

    const tinyobj::attrib_t& attributes = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
    const std::vector<tinyobj::material_t>& materials = reader.GetMaterials();

    vertices.clear();
    indices.clear();

    for (const tinyobj::shape_t& shape : shapes)
    {
        std::size_t indexOffset = 0;

        for (std::size_t faceIndex = 0; faceIndex < shape.mesh.num_face_vertices.size(); ++faceIndex)
        {
            glm::vec3 faceColor{1.0f, 1.0f, 1.0f};
            const int materialId = shape.mesh.material_ids[faceIndex];

            if (materialId >= 0 && static_cast<std::size_t>(materialId) < materials.size())
            {
                const tinyobj::material_t& material = materials[static_cast<std::size_t>(materialId)];
                faceColor = GetDisplayColor(material);
            }

            const int faceVertexCount = static_cast<int>(shape.mesh.num_face_vertices[faceIndex]);
            const std::size_t firstFaceVertex = vertices.size();

            for (int vertexIndex = 0; vertexIndex < faceVertexCount; ++vertexIndex)
            {
                const tinyobj::index_t& index = shape.mesh.indices[indexOffset + static_cast<std::size_t>(vertexIndex)];

                Vertex vertex{};
                vertex.color = faceColor;

                if (index.vertex_index >= 0)
                {
                    const std::size_t positionOffset = static_cast<std::size_t>(index.vertex_index) * 3;
                    vertex.position = {
                        attributes.vertices[positionOffset + 0],
                        attributes.vertices[positionOffset + 1],
                        attributes.vertices[positionOffset + 2]
                    };
                }

                if (index.normal_index >= 0)
                {
                    const std::size_t normalOffset = static_cast<std::size_t>(index.normal_index) * 3;
                    vertex.normal = {
                        attributes.normals[normalOffset + 0],
                        attributes.normals[normalOffset + 1],
                        attributes.normals[normalOffset + 2]
                    };
                }
                else
                {
                    vertex.normal = {};
                }

                if (index.texcoord_index >= 0)
                {
                    const std::size_t texCoordOffset = static_cast<std::size_t>(index.texcoord_index) * 2;
                    vertex.texCoord = {
                        attributes.texcoords[texCoordOffset + 0],
                        attributes.texcoords[texCoordOffset + 1]
                    };
                }

                vertices.push_back(vertex);
                indices.push_back(static_cast<unsigned int>(indices.size()));
            }

            if (faceVertexCount >= 3)
            {
                // Some OBJ files omit normals entirely; use a geometric face normal in that case.
                const glm::vec3 edge1 =
                    vertices[firstFaceVertex + 1].position -
                    vertices[firstFaceVertex].position;
                const glm::vec3 edge2 =
                    vertices[firstFaceVertex + 2].position -
                    vertices[firstFaceVertex].position;
                const glm::vec3 crossProduct = glm::cross(edge1, edge2);
                const float lengthSquared = glm::dot(crossProduct, crossProduct);
                const glm::vec3 faceNormal =
                    lengthSquared > std::numeric_limits<float>::epsilon()
                        ? glm::normalize(crossProduct)
                        : glm::vec3{0.0f, 1.0f, 0.0f};

                for (int vertexIndex = 0; vertexIndex < faceVertexCount; ++vertexIndex)
                {
                    Vertex& vertex = vertices[firstFaceVertex + static_cast<std::size_t>(vertexIndex)];
                    if (glm::dot(vertex.normal, vertex.normal) <=
                        std::numeric_limits<float>::epsilon())
                    {
                        vertex.normal = faceNormal;
                    }
                    else
                    {
                        vertex.normal = glm::normalize(vertex.normal);
                    }
                }
            }

            indexOffset += static_cast<std::size_t>(faceVertexCount);
        }
    }

    return !vertices.empty() && !indices.empty();
}
