#ifndef GLRENDERER_RENDERER_H
#define GLRENDERER_RENDERER_H

#include <SDL3/SDL.h>

class Camera;

class Renderer {
public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    virtual ~Renderer() = default;

    [[nodiscard]] virtual bool Initialize(SDL_Window* window) = 0;
    virtual void Shutdown() = 0;

    virtual void BeginFrame(float red, float green, float blue, float alpha) = 0;
    virtual void Draw(const Camera& camera) = 0;
    virtual void EndFrame() = 0;
};

#endif //GLRENDERER_RENDERER_H
