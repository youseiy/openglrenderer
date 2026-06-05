#include "OpenGLRenderer.h"

#include "Camera.h"
#include "GltfLoader.h"
#include "ObjLoader.h"
#include "Vertex.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {
Vertex vertices[] = {
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
};

unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3
};
}

OpenGLRenderer::~OpenGLRenderer() {
    Shutdown();
}

void OpenGLRenderer::ConfigureContextAttributes() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE
    );
}

SDL_WindowFlags OpenGLRenderer::GetWindowFlags() {
    return static_cast<SDL_WindowFlags>(
        SDL_WINDOW_OPENGL |
        SDL_WINDOW_HIGH_PIXEL_DENSITY |
        SDL_WINDOW_RESIZABLE
    );
}

SDL_GLContext OpenGLRenderer::GetContext() const {
    return m_context;
}

bool OpenGLRenderer::Initialize(SDL_Window* window) {
    m_window = window;

    m_context = SDL_GL_CreateContext(m_window);
    if (!m_context)
    {
        std::cout << "OpenGL context error: "
                  << SDL_GetError()
                  << '\n';

        return false;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        Shutdown();

        return false;
    }

    std::cout << "Vendor: "
              << glGetString(GL_VENDOR)
              << '\n';

    std::cout << "Renderer: "
              << glGetString(GL_RENDERER)
              << '\n';

    std::cout << "Version: "
              << glGetString(GL_VERSION)
              << '\n';

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!CreateShaderProgram())
    {
        Shutdown();

        return false;
    }

    if (!CreateGridResources())
    {
        Shutdown();
        return false;
    }

    CreateGeometry();

    return true;
}

bool OpenGLRenderer::LoadModel(const char* filePath) {
    const std::string extension = std::filesystem::path{filePath}.extension().string();

    // glTF keeps its primitive/material structure; OBJ uses the flat compatibility path.
    if (extension == ".gltf" || extension == ".glb")
    {
        RenderModel model;
        if (!GltfLoader::Load(filePath, model))
        {
            return false;
        }

        m_mesh.Create(std::move(model));
        return true;
    }

    std::vector<Vertex> loadedVertices;
    std::vector<unsigned int> loadedIndices;
    if (!ObjLoader::Load(filePath, loadedVertices, loadedIndices))
    {
        return false;
    }

    m_mesh.Create(
        loadedVertices.data(),
        loadedVertices.size(),
        loadedIndices.data(),
        loadedIndices.size()
    );

    return true;
}

void OpenGLRenderer::SetOpacity(const float opacity) {
    m_opacity = std::clamp(opacity, 0.0f, 1.0f);
}

void OpenGLRenderer::SetPbrMaterial(
    const float metallic,
    const float roughness,
    const float ambientOcclusion
) {
    m_metallic = std::clamp(metallic, 0.0f, 1.0f);
    m_roughness = std::clamp(roughness, 0.045f, 1.0f);
    m_ambientOcclusion = std::clamp(ambientOcclusion, 0.0f, 1.0f);
}

void OpenGLRenderer::SetGridVisible(const bool visible) {
    m_gridVisible = visible;
}

void OpenGLRenderer::Shutdown() {
    m_mesh.Destroy();
    m_shaderProgram.Destroy();
    m_gridShaderProgram.Destroy();

    if (m_gridVao != 0)
    {
        glDeleteVertexArrays(1, &m_gridVao);
        m_gridVao = 0;
    }

    if (m_context)
    {
        SDL_GL_DestroyContext(m_context);
        m_context = nullptr;
    }

    m_window = nullptr;
}

void OpenGLRenderer::BeginFrame(float red, float green, float blue, float alpha) {
    int width{};
    int height{};
    SDL_GetWindowSizeInPixels(m_window, &width, &height);

    glViewport(0, 0, width, height);

    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::Draw(const Camera& camera) {
    // The grid writes real depth, allowing meshes to occlude it normally.
    DrawGrid(camera);

    m_shaderProgram.Use();

    const glm::mat4 model{1.0f};
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::mat4 projection = camera.GetProjectionMatrix();

    m_shaderProgram.SetMat4("uModel", model);
    m_shaderProgram.SetMat4("uView", view);
    m_shaderProgram.SetMat4("uProjection", projection);
    m_shaderProgram.SetVec3("uLightDirection", glm::normalize(glm::vec3{-0.35f, -1.0f, -0.45f}));
    m_shaderProgram.SetVec3("uLightColor", glm::vec3{1.0f});
    m_shaderProgram.SetVec3("uCameraPosition", camera.GetPosition());
    m_shaderProgram.SetFloat("uAmbientStrength", 0.03f);
    m_shaderProgram.SetFloat("uLightIntensity", 2.0f);
    m_mesh.Draw(
        m_shaderProgram,
        m_metallic,
        m_roughness,
        m_ambientOcclusion,
        m_opacity
    );
}

void OpenGLRenderer::EndFrame() {
    SDL_GL_SwapWindow(m_window);
}

bool OpenGLRenderer::CreateShaderProgram() {
    return m_shaderProgram.Load(
        "shaders/basic.vert",
        "shaders/basic.frag"
    );
}

bool OpenGLRenderer::CreateGridResources() {
    if (!m_gridShaderProgram.Load("shaders/grid.vert", "shaders/grid.frag"))
    {
        return false;
    }

    // Core OpenGL requires a bound VAO even when vertices come from gl_VertexID.
    glGenVertexArrays(1, &m_gridVao);
    return true;
}

void OpenGLRenderer::DrawGrid(const Camera& camera) {
    if (!m_gridVisible)
    {
        return;
    }

    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);

    m_gridShaderProgram.Use();
    m_gridShaderProgram.SetMat4("uView", camera.GetViewMatrix());
    m_gridShaderProgram.SetMat4("uProjection", camera.GetProjectionMatrix());
    m_gridShaderProgram.SetVec3("uCameraPosition", camera.GetPosition());

    glBindVertexArray(m_gridVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void OpenGLRenderer::CreateGeometry() {
    if (LoadModel("assets/models/cyprys_house/Cyprys_House.obj"))
    {
        return;
    }

    std::cout << "Falling back to built-in quad mesh\n";
    CreateFallbackGeometry();
}

void OpenGLRenderer::CreateFallbackGeometry() {
    m_mesh.Create(
        vertices,
        sizeof(vertices) / sizeof(vertices[0]),
        indices,
        sizeof(indices) / sizeof(indices[0])
    );
}
