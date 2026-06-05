#ifndef GLRENDERER_ORBITCAMERACONTROLLER_H
#define GLRENDERER_ORBITCAMERACONTROLLER_H

class Camera;

class OrbitCameraController {
public:
    void BeginOrbit(float mouseX, float mouseY);
    void BeginPan(float mouseX, float mouseY);
    void EndDrag();

    void MouseMove(float mouseX, float mouseY, Camera& camera);
    void Zoom(float amount, Camera& camera) const;
    void SetKeyboardMovement(bool moveLeft, bool moveRight, bool moveUp, bool moveDown);
    void Update(float deltaTime, Camera& camera) const;

private:
    enum class DragMode {
        None,
        Orbit,
        Pan
    };

    DragMode m_dragMode{DragMode::None};
    float m_lastMouseX{};
    float m_lastMouseY{};

    float m_orbitSensitivity{0.25f};
    float m_panSensitivity{0.005f};
    float m_zoomSensitivity{0.10f};
    float m_keyboardPanSpeed{1.0f};

    bool m_moveLeft{};
    bool m_moveRight{};
    bool m_moveUp{};
    bool m_moveDown{};
};

#endif //GLRENDERER_ORBITCAMERACONTROLLER_H
