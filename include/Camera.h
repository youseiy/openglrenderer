#ifndef GLRENDERER_CAMERA_H
#define GLRENDERER_CAMERA_H

#include <glm/glm.hpp>

class Camera {
public:
    Camera() = default;

    [[nodiscard]] glm::mat4 GetViewMatrix() const;
    [[nodiscard]] glm::mat4 GetProjectionMatrix() const;
    [[nodiscard]] glm::vec3 GetPosition() const;
    [[nodiscard]] float GetDistance() const;
    [[nodiscard]] float GetMinDistance() const;
    [[nodiscard]] float GetMaxDistance() const;
    [[nodiscard]] float GetFieldOfView() const;

    void SetTarget(const glm::vec3& target);
    void SetAspectRatio(float aspectRatio);
    void SetViewportSize(int width, int height);
    void SetDistance(float distance);
    void SetDistanceLimits(float minDistance, float maxDistance);
    void SetFieldOfView(float fieldOfViewDegrees);
    void SetClippingPlanes(float nearPlane, float farPlane);

    void Orbit(float yawDeltaDegrees, float pitchDeltaDegrees);
    void Pan(float rightDelta, float upDelta);
    void Zoom(float distanceDelta);
    void ZoomByFactor(float factor);

private:
    glm::vec3 m_target{0.0f, 0.0f, 0.0f};
    glm::vec3 m_worldUp{0.0f, 1.0f, 0.0f};

    float m_distance{3.0f};
    float m_minDistance{1.0f};
    float m_maxDistance{500.0f};
    float m_yawDegrees{-90.0f};
    float m_pitchDegrees{0.0f};

    float m_fieldOfViewDegrees{45.0f};
    float m_aspectRatio{16.0f / 9.0f};
    float m_nearPlane{0.1f};
    float m_farPlane{100.0f};
};

#endif //GLRENDERER_CAMERA_H
