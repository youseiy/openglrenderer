#include "OrbitCameraController.h"

#include "Camera.h"

void OrbitCameraController::BeginOrbit(const float mouseX, const float mouseY) {
    m_dragMode = DragMode::Orbit;
    m_lastMouseX = mouseX;
    m_lastMouseY = mouseY;
}

void OrbitCameraController::BeginPan(const float mouseX, const float mouseY) {
    m_dragMode = DragMode::Pan;
    m_lastMouseX = mouseX;
    m_lastMouseY = mouseY;
}

void OrbitCameraController::EndDrag() {
    m_dragMode = DragMode::None;
}

void OrbitCameraController::MouseMove(const float mouseX, const float mouseY, Camera& camera) {
    const float deltaX = mouseX - m_lastMouseX;
    const float deltaY = mouseY - m_lastMouseY;

    m_lastMouseX = mouseX;
    m_lastMouseY = mouseY;

    if (m_dragMode == DragMode::Orbit)
    {
        camera.Orbit(
            deltaX * m_orbitSensitivity,
            -deltaY * m_orbitSensitivity
        );
    }
    else if (m_dragMode == DragMode::Pan)
    {
        camera.Pan(
            -deltaX * m_panSensitivity,
            deltaY * m_panSensitivity
        );
    }
}

void OrbitCameraController::Zoom(const float amount, Camera& camera) const {
    const float zoomFactor = amount > 0.0f
        ? 1.0f - m_zoomSensitivity
        : 1.0f + m_zoomSensitivity;

    camera.ZoomByFactor(zoomFactor);
}

void OrbitCameraController::SetKeyboardMovement(
    const bool moveLeft,
    const bool moveRight,
    const bool moveUp,
    const bool moveDown
) {
    m_moveLeft = moveLeft;
    m_moveRight = moveRight;
    m_moveUp = moveUp;
    m_moveDown = moveDown;
}

void OrbitCameraController::Update(const float deltaTime, Camera& camera) const {
    float horizontal{};
    float vertical{};

    if (m_moveLeft)
    {
        horizontal -= 1.0f;
    }

    if (m_moveRight)
    {
        horizontal += 1.0f;
    }

    if (m_moveUp)
    {
        vertical += 1.0f;
    }

    if (m_moveDown)
    {
        vertical -= 1.0f;
    }

    if (horizontal != 0.0f || vertical != 0.0f)
    {
        camera.Pan(
            horizontal * m_keyboardPanSpeed * deltaTime,
            vertical * m_keyboardPanSpeed * deltaTime
        );
    }
}
