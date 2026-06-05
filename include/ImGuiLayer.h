#ifndef GLRENDERER_IMGUILAYER_H
#define GLRENDERER_IMGUILAYER_H

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

class Camera;

class ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    [[nodiscard]] bool Initialize(SDL_Window* window, SDL_GLContext context);
    void ProcessEvent(const SDL_Event& event);
    void BeginFrame();
    void DrawModelViewerUI(float deltaTime, Camera& camera);
    [[nodiscard]] bool ConsumeSelectedModelPath(std::string& filePath);
    [[nodiscard]] bool IsGridVisible() const;
    [[nodiscard]] glm::vec3 GetBackgroundColor() const;
    void EndFrame();
    void Shutdown();

private:
    void ApplyStyle();
    void UpdateDpiScale();
    [[nodiscard]] float Scale(float value) const;
    void DrawTopBar(float deltaTime);
    void DrawLeftPanel(Camera& camera);
    void DrawRightPanel(Camera& camera);
    void DrawContentDrawer();
    void DrawScenePanel(Camera& camera);
    void DrawModelPanel();
    void DrawRenderPanel();
    void DrawControlsPanel();
    void RefreshModelList();

    bool m_initialized{};
    SDL_Window* m_window{};
    float m_dpiScale{1.0f};
    float m_topBarHeight{};
    float m_bottomPanelHeight{};
    float m_leftPanelWidth{};
    float m_rightPanelWidth{};
    bool m_showDemoWindow{};
    bool m_showGrid{true};
    bool m_wireframe{};
    bool m_vsync{true};
    bool m_enableLighting{true};
    float m_backgroundColor[3]{0.055f, 0.105f, 0.16f};
    float m_lightDirection[3]{-0.4f, -1.0f, -0.2f};
    float m_lightIntensity{1.0f};
    bool m_modelListDirty{true};
    std::string m_selectedModelPath{};
    std::string m_pendingModelPath{};
    std::vector<std::string> m_modelPaths{};
};

#endif //GLRENDERER_IMGUILAYER_H
