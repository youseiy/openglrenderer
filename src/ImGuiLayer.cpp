#include "ImGuiLayer.h"

#include "Camera.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

ImGuiLayer::~ImGuiLayer() {
    Shutdown();
}

bool ImGuiLayer::Initialize(SDL_Window* window, SDL_GLContext context) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    m_window = window;
    ApplyStyle();
    UpdateDpiScale();

    if (!ImGui_ImplSDL3_InitForOpenGL(window, context))
    {
        std::cout << "Failed to initialize ImGui SDL3 backend\n";
        ImGui::DestroyContext();

        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330"))
    {
        std::cout << "Failed to initialize ImGui OpenGL3 backend\n";
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        return false;
    }

    m_initialized = true;

    return true;
}

void ImGuiLayer::ProcessEvent(const SDL_Event& event) {
    if (m_initialized)
    {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED &&
            event.window.windowID == SDL_GetWindowID(m_window))
        {
            // Re-scale when the window moves between monitors with different DPI settings.
            UpdateDpiScale();
        }
    }
}

void ImGuiLayer::BeginFrame() {
    if (!m_initialized)
    {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::DrawModelViewerUI(const float deltaTime, Camera& camera) {
    if (!m_initialized)
    {
        return;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Fixed editor regions leave the center unobstructed for the 3D viewport.
    m_topBarHeight = Scale(42.0f);
    m_bottomPanelHeight = std::min(Scale(220.0f), viewport->WorkSize.y * 0.34f);
    m_leftPanelWidth = std::min(Scale(250.0f), viewport->WorkSize.x * 0.24f);
    m_rightPanelWidth = std::min(Scale(300.0f), viewport->WorkSize.x * 0.28f);

    DrawTopBar(deltaTime);

    if (m_modelListDirty)
    {
        RefreshModelList();
    }

    DrawLeftPanel(camera);
    DrawRightPanel(camera);
    DrawContentDrawer();

    if (m_showDemoWindow)
    {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }
}

bool ImGuiLayer::ConsumeSelectedModelPath(std::string& filePath) {
    if (m_pendingModelPath.empty())
    {
        return false;
    }

    filePath = m_pendingModelPath;
    m_pendingModelPath.clear();

    return true;
}

bool ImGuiLayer::IsGridVisible() const {
    return m_showGrid;
}

glm::vec3 ImGuiLayer::GetBackgroundColor() const {
    return {
        m_backgroundColor[0],
        m_backgroundColor[1],
        m_backgroundColor[2]
    };
}

void ImGuiLayer::EndFrame() {
    if (!m_initialized)
    {
        return;
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::Shutdown() {
    if (!m_initialized)
    {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    m_initialized = false;
    m_window = nullptr;
}

void ImGuiLayer::ApplyStyle() {
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ChildRounding = 1.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(10.0f, 9.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.ItemSpacing = ImVec2(7.0f, 6.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.055f, 0.060f, 0.068f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.045f, 0.050f, 0.058f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.16f, 0.18f, 0.21f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.040f, 0.045f, 0.052f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.070f, 0.075f, 0.085f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.27f, 0.31f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.90f, 0.34f, 0.08f, 0.75f);
    colors[ImGuiCol_Button] = ImVec4(0.12f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.90f, 0.34f, 0.08f, 0.85f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.43f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.43f, 0.12f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.90f, 0.34f, 0.08f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);
}

void ImGuiLayer::UpdateDpiScale() {
    constexpr float UiScale = 1.5f;

    if (!m_window)
    {
        return;
    }

    float newScale = SDL_GetWindowDisplayScale(m_window);
    if (newScale <= 0.0f)
    {
        newScale = 1.0f;
    }

    newScale = std::clamp(newScale * UiScale, UiScale, 4.0f);
    if (std::abs(newScale - m_dpiScale) < 0.01f)
    {
        return;
    }

    // Scale by the ratio to avoid compounding dimensions after multiple monitor changes.
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(newScale / m_dpiScale);
    style.FontScaleDpi = newScale;
    m_dpiScale = newScale;
}

float ImGuiLayer::Scale(const float value) const {
    return value * m_dpiScale;
}

void ImGuiLayer::DrawTopBar(const float deltaTime) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(viewport->WorkSize.x, m_topBarHeight),
        ImGuiCond_Always
    );

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Model Viewer Top Bar", nullptr, flags);

    ImGui::TextColored(ImVec4(1.0f, 0.43f, 0.12f, 1.0f), "GL");
    ImGui::SameLine();
    ImGui::TextUnformatted("Renderer Editor");
    ImGui::SameLine();
    ImGui::TextDisabled("  Level Viewport");

    ImGui::SameLine(Scale(260.0f));
    if (ImGui::Button("Save"))
    {
    }
    ImGui::SameLine();
    if (ImGui::Button("Build"))
    {
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Perspective  |  Lit");

    ImGui::SameLine(ImGui::GetWindowWidth() - Scale(245.0f));
    ImGui::Text("FPS %.1f", deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f);
    ImGui::SameLine();
    ImGui::Text("%.2f ms", deltaTime * 1000.0f);

    ImGui::End();
}

void ImGuiLayer::DrawLeftPanel(Camera& camera) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + m_topBarHeight),
        ImGuiCond_Always
    );
    ImGui::SetNextWindowSize(
        ImVec2(
            m_leftPanelWidth,
            viewport->WorkSize.y - m_topBarHeight - m_bottomPanelHeight
        ),
        ImGuiCond_Always
    );

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("World Outliner", nullptr, flags);
    ImGui::TextDisabled("Search actors");
    ImGui::Separator();

    if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Selectable("Camera", true);
        ImGui::Selectable("Directional Light");
        ImGui::Selectable("Grid");
        ImGui::Selectable(
            m_selectedModelPath.empty() ? "Static Mesh" :
            std::filesystem::path{m_selectedModelPath}.stem().string().c_str()
        );
        ImGui::TreePop();
    }

    ImGui::Spacing();
    DrawModelPanel();
    DrawControlsPanel();
    ImGui::End();
}

void ImGuiLayer::DrawRightPanel(Camera& camera) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x + viewport->WorkSize.x - m_rightPanelWidth,
            viewport->WorkPos.y + m_topBarHeight
        ),
        ImGuiCond_Always
    );
    ImGui::SetNextWindowSize(
        ImVec2(
            m_rightPanelWidth,
            viewport->WorkSize.y - m_topBarHeight - m_bottomPanelHeight
        ),
        ImGuiCond_Always
    );

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Details", nullptr, flags);
    ImGui::TextDisabled("Selected: Camera");
    ImGui::Separator();
    DrawScenePanel(camera);
    DrawRenderPanel();
    ImGui::End();
}

void ImGuiLayer::DrawContentDrawer() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x,
            viewport->WorkPos.y + viewport->WorkSize.y - m_bottomPanelHeight
        ),
        ImGuiCond_Always
    );
    ImGui::SetNextWindowSize(
        ImVec2(viewport->WorkSize.x, m_bottomPanelHeight),
        ImGuiCond_Always
    );

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Content Drawer", nullptr, flags);

    ImGui::BeginChild("ContentFolders", ImVec2(Scale(180.0f), 0.0f), true);
    ImGui::TextUnformatted("Content");
    ImGui::Separator();
    ImGui::Selectable("Assets", true);
    ImGui::Indent();
    ImGui::Selectable("Models");
    ImGui::Unindent();
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("ContentAssets", ImVec2(0.0f, 0.0f), true);
    ImGui::TextDisabled("Content / Models");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Scale(80.0f));
    if (ImGui::Button("Refresh"))
    {
        m_modelListDirty = true;
    }
    ImGui::Separator();

    // The number of asset columns follows the drawer width.
    const float tileWidth = Scale(145.0f);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const int columnCount = std::max(1, static_cast<int>(availableWidth / tileWidth));

    if (ImGui::BeginTable("AssetGrid", columnCount))
    {
        for (const std::string& modelPath : m_modelPaths)
        {
            ImGui::TableNextColumn();
            const std::filesystem::path path{modelPath};
            const bool selected = modelPath == m_selectedModelPath;

            ImGui::PushID(modelPath.c_str());
            if (selected)
            {
                ImGui::PushStyleColor(
                    ImGuiCol_Button,
                    ImVec4(0.90f, 0.34f, 0.08f, 0.75f)
                );
            }

            const std::string buttonLabel =
                "[MESH]\n" + path.stem().string();
            if (ImGui::Button(
                buttonLabel.c_str(),
                ImVec2(tileWidth - Scale(10.0f), Scale(74.0f))
            ))
            {
                m_selectedModelPath = modelPath;
                m_pendingModelPath = modelPath;
            }

            if (selected)
            {
                ImGui::PopStyleColor();
            }
            ImGui::TextDisabled("%s", path.extension().string().c_str());
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::EndChild();
    ImGui::End();
}

void ImGuiLayer::DrawScenePanel(Camera& camera) {
    if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const glm::vec3 position = camera.GetPosition();
        ImGui::Text("Camera position");
        ImGui::Text("X %.2f  Y %.2f  Z %.2f", position.x, position.y, position.z);

        float distance = camera.GetDistance();
        if (ImGui::SliderFloat(
            "Zoom distance",
            &distance,
            camera.GetMinDistance(),
            camera.GetMaxDistance(),
            "%.1f"
        ))
        {
            camera.SetDistance(distance);
        }

        float fieldOfView = camera.GetFieldOfView();
        if (ImGui::SliderFloat("Field of view", &fieldOfView, 15.0f, 90.0f, "%.1f"))
        {
            camera.SetFieldOfView(fieldOfView);
        }

        ImGui::Checkbox("Show grid", &m_showGrid);
        ImGui::ColorEdit3("Background", m_backgroundColor);
    }
}

void ImGuiLayer::DrawModelPanel() {
    if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextUnformatted("Selected model");
        ImGui::TextWrapped("%s", m_selectedModelPath.empty() ? "None" : m_selectedModelPath.c_str());

        if (ImGui::Button("Reload selected") && !m_selectedModelPath.empty())
        {
            m_pendingModelPath = m_selectedModelPath;
        }

        ImGui::SameLine();

        if (ImGui::Button("Refresh"))
        {
            m_modelListDirty = true;
        }
    }
}

void ImGuiLayer::DrawRenderPanel() {
    if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Wireframe", &m_wireframe);
        ImGui::Checkbox("VSync", &m_vsync);
        ImGui::Checkbox("Lighting", &m_enableLighting);
        ImGui::SliderFloat3("Light direction", m_lightDirection, -1.0f, 1.0f);
        ImGui::SliderFloat("Light intensity", &m_lightIntensity, 0.0f, 4.0f);
        ImGui::Checkbox("Show ImGui demo", &m_showDemoWindow);
    }
}

void ImGuiLayer::DrawControlsPanel() {
    if (ImGui::CollapsingHeader("Controls"))
    {
        ImGui::TextUnformatted("Mouse");
        ImGui::BulletText("Left drag: orbit");
        ImGui::BulletText("Right drag: pan");
        ImGui::BulletText("Wheel: zoom");

        ImGui::TextUnformatted("Keyboard");
        ImGui::BulletText("W/A/S/D: pan camera");
    }
}

void ImGuiLayer::RefreshModelList() {
    m_modelPaths.clear();

    const std::filesystem::path root{"assets/models"};

    if (!std::filesystem::exists(root))
    {
        m_modelListDirty = false;
        return;
    }

    for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(root))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const std::filesystem::path extension = entry.path().extension();
        if (extension == ".obj" || extension == ".gltf" || extension == ".glb")
        {
            m_modelPaths.push_back(entry.path().generic_string());
        }
    }

    std::sort(m_modelPaths.begin(), m_modelPaths.end());

    if (m_selectedModelPath.empty())
    {
        const auto cyprysModel = std::find_if(
            m_modelPaths.begin(),
            m_modelPaths.end(),
            [](const std::string& path) {
                return path.find("Cyprys_House.obj") != std::string::npos;
            }
        );

        if (cyprysModel != m_modelPaths.end())
        {
            m_selectedModelPath = *cyprysModel;
        }
        else if (!m_modelPaths.empty())
        {
            m_selectedModelPath = m_modelPaths.front();
        }
    }

    m_modelListDirty = false;
}
