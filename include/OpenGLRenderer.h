#ifndef GLRENDERER_OPENGLRENDERER_H
#define GLRENDERER_OPENGLRENDERER_H

#include "Mesh.h"
#include "Renderer.h"
#include "ShaderProgram.h"

class OpenGLRenderer final : public Renderer {
public:
    OpenGLRenderer() = default;
    ~OpenGLRenderer() override;

    OpenGLRenderer(const OpenGLRenderer&) = delete;
    OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;

    static void ConfigureContextAttributes();
    [[nodiscard]] static SDL_WindowFlags GetWindowFlags();
    [[nodiscard]] SDL_GLContext GetContext() const;

    [[nodiscard]] bool Initialize(SDL_Window* window) override;
    void Shutdown() override;
    [[nodiscard]] bool LoadModel(const char* filePath);
    void SetOpacity(float opacity);
    void SetPbrMaterial(float metallic, float roughness, float ambientOcclusion = 1.0f);
    void SetGridVisible(bool visible);

    void BeginFrame(float red, float green, float blue, float alpha) override;
    void Draw(const Camera& camera) override;
    void EndFrame() override;

private:
    [[nodiscard]] bool CreateShaderProgram();
    [[nodiscard]] bool CreateGridResources();
    void DrawGrid(const Camera& camera);
    void CreateGeometry();
    void CreateFallbackGeometry();

    SDL_Window* m_window{};
    SDL_GLContext m_context{};

    ShaderProgram m_shaderProgram{};
    ShaderProgram m_gridShaderProgram{};
    Mesh m_mesh{};
    unsigned int m_gridVao{};
    bool m_gridVisible{true};
    float m_opacity{1.0f};
    float m_metallic{};
    float m_roughness{0.5f};
    float m_ambientOcclusion{1.0f};
};

#endif //GLRENDERER_OPENGLRENDERER_H
