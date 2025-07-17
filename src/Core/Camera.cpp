#include "Camera.h"
#include "InputManager.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

namespace VaporFrame {
namespace Core {

// Camera Implementation
Camera::Camera(CameraType type)
    : type(type)
    , position(0.0f, 0.0f, 3.0f)
    , target(0.0f, 0.0f, 0.0f)
    , up(0.0f, 1.0f, 0.0f)
    , front(0.0f, 0.0f, -1.0f)
    , right(1.0f, 0.0f, 0.0f)
    , fov(45.0f)
    , aspectRatio(16.0f / 9.0f)
    , nearPlane(0.1f)
    , farPlane(100.0f)
    , orthographicSize(10.0f)
    , movementSpeed(5.0f)
    , rotationSpeed(1.0f)
    , mouseSensitivity(0.1f)
    , mouseLookEnabled(false)
    , keyboardMovementEnabled(false)
    , firstMouse(true)
    , lastMouseX(0.0f)
    , lastMouseY(0.0f)
    , yaw(-90.0f)
    , pitch(0.0f)
    , orbitMode(false)
    , orbitCenter(0.0f, 0.0f, 0.0f)
    , orbitDistance(5.0f)
    , frustumValid(false)
    , viewMatrixDirty(true)
    , projectionMatrixDirty(true) {
    
    updateVectors();
    VF_LOG_DEBUG("Camera created with type: {}", type == CameraType::Perspective ? "Perspective" : "Orthographic");
}

void Camera::setPosition(const glm::vec3& pos) {
    position = pos;
    viewMatrixDirty = true;
    frustumValid = false;
}

void Camera::setTarget(const glm::vec3& tgt) {
    target = tgt;
    viewMatrixDirty = true;
    frustumValid = false;
}

void Camera::setUp(const glm::vec3& u) {
    up = glm::normalize(u);
    viewMatrixDirty = true;
}

void Camera::setFOV(float f) {
    fov = f;
    projectionMatrixDirty = true;
    frustumValid = false;
}

void Camera::setAspectRatio(float aspect) {
    aspectRatio = aspect;
    projectionMatrixDirty = true;
    frustumValid = false;
}

void Camera::setNearPlane(float near) {
    nearPlane = near;
    projectionMatrixDirty = true;
    frustumValid = false;
}

void Camera::setFarPlane(float far) {
    farPlane = far;
    projectionMatrixDirty = true;
    frustumValid = false;
}

void Camera::setOrthographicSize(float size) {
    orthographicSize = size;
    projectionMatrixDirty = true;
    frustumValid = false;
}

void Camera::move(const glm::vec3& offset) {
    position += offset;
    viewMatrixDirty = true;
    frustumValid = false;
}

void Camera::move(CameraMovement direction, float distance) {
    float velocity = movementSpeed * distance;
    
    switch (direction) {
        case CameraMovement::Forward:
            position += front * velocity;
            break;
        case CameraMovement::Backward:
            position -= front * velocity;
            break;
        case CameraMovement::Left:
            position -= right * velocity;
            break;
        case CameraMovement::Right:
            position += right * velocity;
            break;
        case CameraMovement::Up:
            position += up * velocity;
            break;
        case CameraMovement::Down:
            position -= up * velocity;
            break;
    }
    
    viewMatrixDirty = true;
    frustumValid = false;
}

void Camera::rotate(float yawOffset, float pitchOffset) {
    yaw += yawOffset * mouseSensitivity;
    pitch += pitchOffset * mouseSensitivity;
    
    constrainPitch();
    updateVectors();
    viewMatrixDirty = true;
    frustumValid = false;
}

void Camera::rotateAroundTarget(float yawOffset, float pitchOffset) {
    yaw += yawOffset * mouseSensitivity;
    pitch += pitchOffset * mouseSensitivity;
    
    constrainPitch();
    
    // Calculate new position based on spherical coordinates
    float x = orbitDistance * cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    float y = orbitDistance * sin(glm::radians(pitch));
    float z = orbitDistance * sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    
    position = orbitCenter + glm::vec3(x, y, z);
    target = orbitCenter;
    
    updateVectors();
    viewMatrixDirty = true;
    frustumValid = false;
}

void Camera::lookAt(const glm::vec3& tgt) {
    target = tgt;
    front = glm::normalize(target - position);
    right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front));
    
    viewMatrixDirty = true;
    frustumValid = false;
}

void Camera::orbit(const glm::vec3& center, float distance, float yawAngle, float pitchAngle) {
    orbitCenter = center;
    orbitDistance = distance;
    yaw = yawAngle;
    pitch = pitchAngle;
    
    constrainPitch();
    
    float x = distance * cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    float y = distance * sin(glm::radians(pitch));
    float z = distance * sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    
    position = center + glm::vec3(x, y, z);
    target = center;
    
    updateVectors();
    viewMatrixDirty = true;
    frustumValid = false;
}

void Camera::enableMouseLook(bool enable) {
    mouseLookEnabled = enable;
    if (enable) {
        firstMouse = true;
    }
}

void Camera::enableKeyboardMovement(bool enable) {
    keyboardMovementEnabled = enable;
}

void Camera::setMouseSensitivity(float sensitivity) {
    mouseSensitivity = sensitivity;
}

void Camera::setMovementSpeed(float speed) {
    movementSpeed = speed;
}

void Camera::setRotationSpeed(float speed) {
    rotationSpeed = speed;
}

void Camera::update(float deltaTime) {
    // Handle mouse look if enabled
    if (mouseLookEnabled) {
        double mouseX, mouseY;
        InputManager::getInstance().getMousePosition(mouseX, mouseY);
        
        if (firstMouse) {
            lastMouseX = static_cast<float>(mouseX);
            lastMouseY = static_cast<float>(mouseY);
            firstMouse = false;
        }
        
        float xOffset = static_cast<float>(mouseX) - lastMouseX;
        float yOffset = lastMouseY - static_cast<float>(mouseY);
        
        lastMouseX = static_cast<float>(mouseX);
        lastMouseY = static_cast<float>(mouseY);
        
        if (orbitMode) {
            rotateAroundTarget(xOffset, yOffset);
        } else {
            rotate(xOffset, yOffset);
        }
    }
    
    // Handle keyboard movement if enabled
    if (keyboardMovementEnabled) {
        if (IsKeyHeld(KeyCode::W)) {
            move(CameraMovement::Forward, deltaTime);
        }
        if (IsKeyHeld(KeyCode::S)) {
            move(CameraMovement::Backward, deltaTime);
        }
        if (IsKeyHeld(KeyCode::A)) {
            move(CameraMovement::Left, deltaTime);
        }
        if (IsKeyHeld(KeyCode::D)) {
            move(CameraMovement::Right, deltaTime);
        }
        if (IsKeyHeld(KeyCode::Q)) {
            move(CameraMovement::Down, deltaTime);
        }
        if (IsKeyHeld(KeyCode::E)) {
            move(CameraMovement::Up, deltaTime);
        }
    }
}

glm::mat4 Camera::getViewMatrix() const {
    if (viewMatrixDirty) {
        viewMatrix = glm::lookAt(position, position + front, up);
        viewMatrixDirty = false;
    }
    return viewMatrix;
}

glm::mat4 Camera::getProjectionMatrix() const {
    if (projectionMatrixDirty) {
        if (type == CameraType::Perspective) {
            projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        } else {
            float halfSize = orthographicSize * 0.5f;
            projectionMatrix = glm::ortho(-halfSize * aspectRatio, halfSize * aspectRatio, 
                                        -halfSize, halfSize, nearPlane, farPlane);
        }
        projectionMatrixDirty = false;
    }
    return projectionMatrix;
}

glm::mat4 Camera::getViewProjectionMatrix() const {
    return getProjectionMatrix() * getViewMatrix();
}

bool Camera::isPointInFrustum(const glm::vec3& point) const {
    if (!frustumValid) {
        calculateFrustumPlanes();
    }
    
    for (const auto& plane : frustumPlanes) {
        if (glm::dot(plane.normal, point) + plane.distance < 0) {
            return false;
        }
    }
    return true;
}

bool Camera::isSphereInFrustum(const glm::vec3& center, float radius) const {
    if (!frustumValid) {
        calculateFrustumPlanes();
    }
    
    for (const auto& plane : frustumPlanes) {
        if (glm::dot(plane.normal, center) + plane.distance < -radius) {
            return false;
        }
    }
    return true;
}

bool Camera::isBoxInFrustum(const glm::vec3& min, const glm::vec3& max) const {
    if (!frustumValid) {
        calculateFrustumPlanes();
    }
    
    for (const auto& plane : frustumPlanes) {
        glm::vec3 positive = max;
        glm::vec3 negative = min;
        
        if (plane.normal.x >= 0) {
            positive.x = max.x;
            negative.x = min.x;
        } else {
            positive.x = min.x;
            negative.x = max.x;
        }
        
        if (plane.normal.y >= 0) {
            positive.y = max.y;
            negative.y = min.y;
        } else {
            positive.y = min.y;
            negative.y = max.y;
        }
        
        if (plane.normal.z >= 0) {
            positive.z = max.z;
            negative.z = min.z;
        } else {
            positive.z = min.z;
            negative.z = max.z;
        }
        
        if (glm::dot(plane.normal, positive) + plane.distance < 0) {
            return false;
        }
    }
    return true;
}

void Camera::setFirstPersonMode() {
    orbitMode = false;
    mouseLookEnabled = true;
    keyboardMovementEnabled = true;
    VF_LOG_DEBUG("Camera set to first person mode");
}

void Camera::setThirdPersonMode() {
    orbitMode = true;
    mouseLookEnabled = true;
    keyboardMovementEnabled = true;
    VF_LOG_DEBUG("Camera set to third person mode");
}

void Camera::setOrbitMode() {
    orbitMode = true;
    mouseLookEnabled = true;
    keyboardMovementEnabled = false;
    VF_LOG_DEBUG("Camera set to orbit mode");
}

void Camera::setTopDownMode() {
    orbitMode = false;
    mouseLookEnabled = false;
    keyboardMovementEnabled = true;
    position = glm::vec3(0.0f, 10.0f, 0.0f);
    target = glm::vec3(0.0f, 0.0f, 0.0f);
    up = glm::vec3(0.0f, 0.0f, -1.0f);
    updateVectors();
    VF_LOG_DEBUG("Camera set to top down mode");
}

void Camera::bindInputControls(InputManager& inputManager) {
    // Bind mouse look
    inputManager.bindAction("CameraMouseLook", InputDevice::Mouse, 
                           static_cast<int>(KeyCode::MouseRight), InputAction::Hold,
                           [this]() { enableMouseLook(true); });
    
    // Bind movement keys
    inputManager.bindAction("CameraForward", InputDevice::Keyboard, 
                           static_cast<int>(KeyCode::W), InputAction::Hold,
                           [this]() { /* Handled in update() */ });
    
    inputManager.bindAction("CameraBackward", InputDevice::Keyboard, 
                           static_cast<int>(KeyCode::S), InputAction::Hold,
                           [this]() { /* Handled in update() */ });
    
    inputManager.bindAction("CameraLeft", InputDevice::Keyboard, 
                           static_cast<int>(KeyCode::A), InputAction::Hold,
                           [this]() { /* Handled in update() */ });
    
    inputManager.bindAction("CameraRight", InputDevice::Keyboard, 
                           static_cast<int>(KeyCode::D), InputAction::Hold,
                           [this]() { /* Handled in update() */ });
    
    VF_LOG_DEBUG("Camera input controls bound");
}

void Camera::unbindInputControls() {
    InputManager& inputManager = InputManager::getInstance();
    inputManager.unbindAction("CameraMouseLook");
    inputManager.unbindAction("CameraForward");
    inputManager.unbindAction("CameraBackward");
    inputManager.unbindAction("CameraLeft");
    inputManager.unbindAction("CameraRight");
    VF_LOG_DEBUG("Camera input controls unbound");
}

void Camera::updateVectors() {
    front = glm::normalize(glm::vec3(
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    ));
    
    right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::constrainPitch() {
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

void Camera::calculateFrustumPlanes() const {
    glm::mat4 vp = getViewProjectionMatrix();
    
    // Left plane
    frustumPlanes[0].normal.x = vp[0][3] + vp[0][0];
    frustumPlanes[0].normal.y = vp[1][3] + vp[1][0];
    frustumPlanes[0].normal.z = vp[2][3] + vp[2][0];
    frustumPlanes[0].distance = vp[3][3] + vp[3][0];
    
    // Right plane
    frustumPlanes[1].normal.x = vp[0][3] - vp[0][0];
    frustumPlanes[1].normal.y = vp[1][3] - vp[1][0];
    frustumPlanes[1].normal.z = vp[2][3] - vp[2][0];
    frustumPlanes[1].distance = vp[3][3] - vp[3][0];
    
    // Bottom plane
    frustumPlanes[2].normal.x = vp[0][3] + vp[0][1];
    frustumPlanes[2].normal.y = vp[1][3] + vp[1][1];
    frustumPlanes[2].normal.z = vp[2][3] + vp[2][1];
    frustumPlanes[2].distance = vp[3][3] + vp[3][1];
    
    // Top plane
    frustumPlanes[3].normal.x = vp[0][3] - vp[0][1];
    frustumPlanes[3].normal.y = vp[1][3] - vp[1][1];
    frustumPlanes[3].normal.z = vp[2][3] - vp[2][1];
    frustumPlanes[3].distance = vp[3][3] - vp[3][1];
    
    // Near plane
    frustumPlanes[4].normal.x = vp[0][3] + vp[0][2];
    frustumPlanes[4].normal.y = vp[1][3] + vp[1][2];
    frustumPlanes[4].normal.z = vp[2][3] + vp[2][2];
    frustumPlanes[4].distance = vp[3][3] + vp[3][2];
    
    // Far plane
    frustumPlanes[5].normal.x = vp[0][3] - vp[0][2];
    frustumPlanes[5].normal.y = vp[1][3] - vp[1][2];
    frustumPlanes[5].normal.z = vp[2][3] - vp[2][2];
    frustumPlanes[5].distance = vp[3][3] - vp[3][2];
    
    // Normalize all planes
    for (auto& plane : frustumPlanes) {
        float length = glm::length(plane.normal);
        if (length > 0.0f) {
            plane.normal = plane.normal / length;
            plane.distance = plane.distance / length;
        }
    }
    
    frustumValid = true;
}

// CameraController Implementation
CameraController::CameraController(std::shared_ptr<Camera> cam)
    : camera(cam)
    , followTarget(0.0f, 0.0f, 0.0f)
    , followDistance(5.0f)
    , followMode(false) {
    
    // Initialize movement state
    movementState[CameraMovement::Forward] = false;
    movementState[CameraMovement::Backward] = false;
    movementState[CameraMovement::Left] = false;
    movementState[CameraMovement::Right] = false;
    movementState[CameraMovement::Up] = false;
    movementState[CameraMovement::Down] = false;
}

void CameraController::update(float deltaTime) {
    if (!camera) return;
    
    // Update camera
    camera->update(deltaTime);
    
    // Handle follow mode
    if (followMode) {
        glm::vec3 targetPos = followTarget;
        glm::vec3 currentPos = camera->getPosition();
        glm::vec3 direction = glm::normalize(currentPos - targetPos);
        glm::vec3 newPos = targetPos + direction * followDistance;
        camera->setPosition(newPos);
        camera->lookAt(targetPos);
    }
}

void CameraController::setFreeLookMode() {
    if (!camera) return;
    camera->setFirstPersonMode();
    followMode = false;
}

void CameraController::setOrbitMode(const glm::vec3& center) {
    if (!camera) return;
    camera->setOrbitMode();
    camera->orbit(center, camera->getPosition().length(), 0.0f, 0.0f);
}

void CameraController::setFollowMode(const glm::vec3& target, float distance) {
    if (!camera) return;
    followTarget = target;
    followDistance = distance;
    followMode = true;
    camera->enableMouseLook(false);
    camera->enableKeyboardMovement(false);
}

void CameraController::handleMouseMovement(double xOffset, double yOffset) {
    if (!camera) return;
    camera->rotate(static_cast<float>(xOffset), static_cast<float>(yOffset));
}

void CameraController::handleMouseScroll(double yOffset) {
    if (!camera) return;
    // Adjust FOV or movement speed based on scroll
    float currentFOV = camera->getFOV();
    currentFOV -= static_cast<float>(yOffset);
    currentFOV = std::clamp(currentFOV, 1.0f, 90.0f);
    camera->setFOV(currentFOV);
}

void CameraController::handleKeyboardInput(CameraMovement direction, bool pressed) {
    movementState[direction] = pressed;
}

} // namespace Core
} // namespace VaporFrame 