#include "Camera.h"
#include "ImGuiLayer.h"
#include "OpenGLRenderer.h"
#include "OrbitCameraController.h"
#include "Renderer.h"

#include <imgui.h>
#include <SDL3/SDL.h>

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cout << "SDL Init Error: "
                  << SDL_GetError()
                  << '\n';

        return -1;
    }

    OpenGLRenderer::ConfigureContextAttributes();

    SDL_Window* window = SDL_CreateWindow(
        "SDL3 + OpenGL",
        1280,
        720,
        OpenGLRenderer::GetWindowFlags()
    );

    if (!window)
    {
        std::cout << "Window Error: "
                  << SDL_GetError()
                  << '\n';

        SDL_Quit();

        return -1;
    }

    OpenGLRenderer openGLRenderer;
    Renderer* renderer = &openGLRenderer;
    Camera camera;
    camera.SetDistance(180.0f);
    camera.SetClippingPlanes(0.1f, 1000.0f);
    OrbitCameraController cameraController;
    ImGuiLayer imguiLayer;

    if (!renderer->Initialize(window))
    {
        SDL_DestroyWindow(window);
        SDL_Quit();

        return -1;
    }

    if (!imguiLayer.Initialize(window, openGLRenderer.GetContext()))
    {
        renderer->Shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();

        return -1;
    }

    bool running = true;
    bool moveLeft = false;
    bool moveRight = false;
    bool moveUp = false;
    bool moveDown = false;

    Uint64 previousTicks = SDL_GetTicks();

    while (running)
    {
        const Uint64 currentTicks = SDL_GetTicks();
        const float deltaTime = static_cast<float>(currentTicks - previousTicks) / 1000.0f;
        previousTicks = currentTicks;

        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            imguiLayer.ProcessEvent(event);

            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }

            const ImGuiIO& io = ImGui::GetIO();
            const bool mouseCaptured = io.WantCaptureMouse;
            const bool keyboardCaptured = io.WantCaptureKeyboard;

            if (!mouseCaptured && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    cameraController.BeginOrbit(event.button.x, event.button.y);
                }
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    cameraController.BeginPan(event.button.x, event.button.y);
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                cameraController.EndDrag();
            }
            else if (!mouseCaptured && event.type == SDL_EVENT_MOUSE_MOTION)
            {
                cameraController.MouseMove(event.motion.x, event.motion.y, camera);
            }
            else if (!mouseCaptured && event.type == SDL_EVENT_MOUSE_WHEEL)
            {
                cameraController.Zoom(event.wheel.y, camera);
            }
            else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
            {
                const bool isPressed = event.type == SDL_EVENT_KEY_DOWN;

                if (keyboardCaptured && isPressed)
                {
                    continue;
                }

                if (event.key.key == SDLK_A)
                {
                    moveLeft = isPressed;
                }
                else if (event.key.key == SDLK_D)
                {
                    moveRight = isPressed;
                }
                else if (event.key.key == SDLK_W)
                {
                    moveUp = isPressed;
                }
                else if (event.key.key == SDLK_S)
                {
                    moveDown = isPressed;
                }

                cameraController.SetKeyboardMovement(
                    moveLeft,
                    moveRight,
                    moveUp,
                    moveDown
                );
            }
        }

        int width{};
        int height{};
        SDL_GetWindowSizeInPixels(window, &width, &height);
        camera.SetViewportSize(width, height);
        cameraController.Update(deltaTime, camera);

        imguiLayer.BeginFrame();
        imguiLayer.DrawModelViewerUI(deltaTime, camera);

        std::string selectedModelPath;
        if (imguiLayer.ConsumeSelectedModelPath(selectedModelPath))
        {
            if (openGLRenderer.LoadModel(selectedModelPath.c_str()))
            {
                camera.SetDistance(180.0f);
            }
        }

        const glm::vec3 backgroundColor = imguiLayer.GetBackgroundColor();
        openGLRenderer.SetGridVisible(imguiLayer.IsGridVisible());
        renderer->BeginFrame(
            backgroundColor.r,
            backgroundColor.g,
            backgroundColor.b,
            1.0f
        );
        renderer->Draw(camera);
        imguiLayer.EndFrame();
        renderer->EndFrame();
    }

    imguiLayer.Shutdown();
    renderer->Shutdown();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
