#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {
constexpr float MinPitchDegrees = -89.0f;
constexpr float MaxPitchDegrees = 89.0f;
constexpr float MinFieldOfViewDegrees = 1.0f;
constexpr float MaxFieldOfViewDegrees = 120.0f;
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(GetPosition(), m_target, m_worldUp);
}

glm::mat4 Camera::GetProjectionMatrix() const {
    return glm::perspective(
        glm::radians(m_fieldOfViewDegrees),
        m_aspectRatio,
        m_nearPlane,
        m_farPlane
    );
}

glm::vec3 Camera::GetPosition() const {
    // Orbit angles describe the direction from the camera toward its target.
    const float yaw = glm::radians(m_yawDegrees);
    const float pitch = glm::radians(m_pitchDegrees);

    glm::vec3 direction{};
    direction.x = std::cos(yaw) * std::cos(pitch);
    direction.y = std::sin(pitch);
    direction.z = std::sin(yaw) * std::cos(pitch);

    return m_target - glm::normalize(direction) * m_distance;
}

float Camera::GetDistance() const {
    return m_distance;
}

float Camera::GetMinDistance() const {
    return m_minDistance;
}

float Camera::GetMaxDistance() const {
    return m_maxDistance;
}

float Camera::GetFieldOfView() const {
    return m_fieldOfViewDegrees;
}

void Camera::SetTarget(const glm::vec3& target) {
    m_target = target;
}

void Camera::SetAspectRatio(const float aspectRatio) {
    if (aspectRatio > 0.0f)
    {
        m_aspectRatio = aspectRatio;
    }
}

void Camera::SetViewportSize(const int width, const int height) {
    if (width > 0 && height > 0)
    {
        m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    }
}

void Camera::SetDistance(const float distance) {
    m_distance = std::clamp(distance, m_minDistance, m_maxDistance);
}

void Camera::SetDistanceLimits(const float minDistance, const float maxDistance) {
    if (minDistance > 0.0f && maxDistance >= minDistance)
    {
        m_minDistance = minDistance;
        m_maxDistance = maxDistance;
        SetDistance(m_distance);
    }
}

void Camera::SetFieldOfView(const float fieldOfViewDegrees) {
    m_fieldOfViewDegrees = std::clamp(
        fieldOfViewDegrees,
        MinFieldOfViewDegrees,
        MaxFieldOfViewDegrees
    );
}

void Camera::SetClippingPlanes(const float nearPlane, const float farPlane) {
    if (nearPlane > 0.0f && farPlane > nearPlane)
    {
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
    }
}

void Camera::Orbit(const float yawDeltaDegrees, const float pitchDeltaDegrees) {
    m_yawDegrees += yawDeltaDegrees;
    m_pitchDegrees = std::clamp(
        m_pitchDegrees + pitchDeltaDegrees,
        MinPitchDegrees,
        MaxPitchDegrees
    );
}

void Camera::Pan(const float rightDelta, const float upDelta) {
    // Build a camera-relative basis so panning remains intuitive after orbiting.
    const glm::vec3 forward = glm::normalize(m_target - GetPosition());
    const glm::vec3 right = glm::normalize(glm::cross(forward, m_worldUp));
    const glm::vec3 up = glm::normalize(glm::cross(right, forward));

    m_target += right * rightDelta;
    m_target += up * upDelta;
}

void Camera::Zoom(const float distanceDelta) {
    SetDistance(m_distance + distanceDelta);
}

void Camera::ZoomByFactor(const float factor) {
    if (factor > 0.0f)
    {
        SetDistance(m_distance * factor);
    }
}
