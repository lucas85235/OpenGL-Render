#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraMode { FPS, ORBIT };

enum class ProjectionType { PERSPECTIVE, ORTHOGRAPHIC };

class Camera {
public:
  enum Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

private:
  glm::vec3 position;
  glm::vec3 front;
  glm::vec3 up;
  glm::vec3 right;
  glm::vec3 worldUp;

  float yaw;
  float pitch;

  float movementSpeed;
  float mouseSensitivity;
  float fov;
  float nearPlane;
  float farPlane;
  float orthoSize;

  CameraMode mode;
  ProjectionType projectionType;

  glm::vec3 orbitTarget;
  float orbitDistance;

  void updateVectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
  }

public:
  Camera(CameraMode cameraMode = CameraMode::FPS)
      : position(0.0f, 5.0f, 10.0f), front(0.0f, 0.0f, -1.0f),
        worldUp(0.0f, 1.0f, 0.0f), yaw(-90.0f), pitch(0.0f),
        movementSpeed(5.0f), mouseSensitivity(0.1f), fov(45.0f),
        nearPlane(0.1f), farPlane(1000.0f), orthoSize(10.0f), mode(cameraMode),
        projectionType(ProjectionType::PERSPECTIVE), orbitTarget(0.0f),
        orbitDistance(10.0f) {
    updateVectors();
  }

  void SetPosition(const glm::vec3 &pos) {
    position = pos;
    if (mode == CameraMode::ORBIT) {
      orbitDistance = glm::length(position - orbitTarget);
    }
  }

  void SetTarget(const glm::vec3 &target) {
    orbitTarget = target;
    if (mode == CameraMode::ORBIT) {
      glm::vec3 dir = position - orbitTarget;
      orbitDistance = glm::length(dir);
      if (orbitDistance > 0.001f) {
        dir = glm::normalize(dir);
        pitch = glm::degrees(asin(dir.y));
        yaw = glm::degrees(atan2(dir.z, dir.x));
      }
    }
  }

  void SetMode(CameraMode newMode) {
    mode = newMode;
    if (mode == CameraMode::ORBIT) {
      orbitDistance = glm::length(position - orbitTarget);
    }
  }

  void SetProjectionType(ProjectionType type) { projectionType = type; }

  void ToggleProjection() {
    projectionType = (projectionType == ProjectionType::PERSPECTIVE)
                         ? ProjectionType::ORTHOGRAPHIC
                         : ProjectionType::PERSPECTIVE;
  }

  void SetFOV(float newFov) { fov = glm::clamp(newFov, 1.0f, 120.0f); }
  void SetNearFar(float near, float far) {
    nearPlane = near;
    farPlane = far;
  }
  void SetSpeed(float speed) { movementSpeed = speed; }
  void SetSensitivity(float sens) { mouseSensitivity = sens; }

  void ProcessKeyboard(Movement direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;

    if (mode == CameraMode::FPS) {
      switch (direction) {
      case FORWARD:
        position += front * velocity;
        break;
      case BACKWARD:
        position -= front * velocity;
        break;
      case LEFT:
        position -= right * velocity;
        break;
      case RIGHT:
        position += right * velocity;
        break;
      case UP:
        position += worldUp * velocity;
        break;
      case DOWN:
        position -= worldUp * velocity;
        break;
      }
    } else {
      switch (direction) {
      case FORWARD:
        orbitDistance -= velocity;
        break;
      case BACKWARD:
        orbitDistance += velocity;
        break;
      case LEFT:
        yaw -= velocity * 10.0f;
        break;
      case RIGHT:
        yaw += velocity * 10.0f;
        break;
      case UP:
        pitch += velocity * 10.0f;
        break;
      case DOWN:
        pitch -= velocity * 10.0f;
        break;
      }
      orbitDistance = glm::max(orbitDistance, 1.0f);
      pitch = glm::clamp(pitch, -89.0f, 89.0f);
      updateOrbitPosition();
    }
  }

  void ProcessMouse(float xOffset, float yOffset, bool constrainPitch = true) {
    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    yaw += xOffset;
    pitch += yOffset;

    if (constrainPitch) {
      pitch = glm::clamp(pitch, -89.0f, 89.0f);
    }

    updateVectors();

    if (mode == CameraMode::ORBIT) {
      updateOrbitPosition();
    }
  }

  void ProcessScroll(float yOffset) {
    if (mode == CameraMode::ORBIT) {
      orbitDistance -= yOffset;
      orbitDistance = glm::max(orbitDistance, 1.0f);
      updateOrbitPosition();
    } else {
      fov -= yOffset;
      fov = glm::clamp(fov, 1.0f, 120.0f);
    }
  }

  glm::mat4 GetViewMatrix() const {
    if (mode == CameraMode::ORBIT) {
      return glm::lookAt(position, orbitTarget, worldUp);
    }
    return glm::lookAt(position, position + front, up);
  }

  glm::mat4 GetProjectionMatrix(float aspect) const {
    if (projectionType == ProjectionType::ORTHOGRAPHIC) {
      float halfWidth = orthoSize * aspect;
      float halfHeight = orthoSize;
      return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight,
                        nearPlane, farPlane);
    }
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
  }

  glm::vec3 GetPosition() const { return position; }
  glm::vec3 GetFront() const { return front; }
  glm::vec3 GetUp() const { return up; }
  glm::vec3 GetRight() const { return right; }
  float GetFOV() const { return fov; }
  float GetYaw() const { return yaw; }
  float GetPitch() const { return pitch; }
  CameraMode GetMode() const { return mode; }
  ProjectionType GetProjectionType() const { return projectionType; }

private:
  void updateOrbitPosition() {
    float x = orbitDistance * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    float y = orbitDistance * sin(glm::radians(pitch));
    float z = orbitDistance * cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    position = orbitTarget + glm::vec3(x, y, z);
    front = glm::normalize(orbitTarget - position);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
  }
};

#endif // CAMERA_HPP
