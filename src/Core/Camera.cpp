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
    , cameraMode(CameraMode::Game)
    , position(0.0f, 0.0f, 3.0f)
    , target(0.0f, 0.0f, 0.0f)
    , up(0.0f, 1.0f, 0.0f)
    , front(0.0f, 0.0f, -1.0f)
    , right(1.0f, 0.0f, 0.0f)
    , fov(90.0f)  // UE5 default horizontal FOV
    , aspectRatio(16.0f / 9.0f)
    , nearPlane(0.1f)
    , farPlane(100.0f)
    , orthographicSize(10.0f)
    , movementSpeed(5.0f)
    , rotationSpeed(1.0f)
    , mouseSensitivity(0.1f)
    , acceleration(50.0f)  // Much faster acceleration for responsive movement
    , deceleration(20.0f)  // Faster deceleration
    , mouseLookEnabled(false)
    , keyboardMovementEnabled(false)
    , firstMouse(true)
    , lastMouseX(0.0f)
    , lastMouseY(0.0f)
    , yaw(-90.0f)
    , pitch(0.0f)
    , roll(0.0f)
    , orbitMode(false)
    , orbitCenter(0.0f, 0.0f, 0.0f)
    , orbitDistance(5.0f)
    , orbitYaw(-90.0f)
    , orbitPitch(0.0f)
    , velocity(0.0f)
    , targetVelocity(0.0f)
    , frustumValid(false)
    , viewMatrixDirty(true)
    , projectionMatrixDirty(true) {
    
    updateVectors();
    VF_LOG_DEBUG("Camera created with type: {} and mode: {}", 
                 type == CameraType::Perspective ? "Perspective" : "Orthographic",
                 cameraMode == CameraMode::Game ? "Game" : cameraMode == CameraMode::Editor ? "Editor" : "Cinematic");
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

void Camera::setCameraMode(CameraMode mode) {
    cameraMode = mode;
    if (mode == CameraMode::Editor) {
        orbitMode = true;
        orbitCenter = target;
        orbitDistance = glm::length(position - target);
        orbitYaw = yaw;
        orbitPitch = pitch;
    } else {
        orbitMode = false;
    }
    VF_LOG_DEBUG("Camera mode changed to: {}", 
                 mode == CameraMode::Game ? "Game" : mode == CameraMode::Editor ? "Editor" : "Cinematic");
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
            targetVelocity += front * velocity;
            break;
        case CameraMovement::Backward:
            targetVelocity -= front * velocity;
            break;
        case CameraMovement::Left:
            targetVelocity -= right * velocity;
            break;
        case CameraMovement::Right:
            targetVelocity += right * velocity;
            break;
        case CameraMovement::Up:
            targetVelocity += up * velocity;
            break;
        case CameraMovement::Down:
            targetVelocity -= up * velocity;
            break;
    }
}

void Camera::rotate(float yawOffset, float pitchOffset, float rollOffset) {
    yaw += yawOffset * mouseSensitivity;
    pitch += pitchOffset * mouseSensitivity;
    roll += rollOffset * mouseSensitivity;
    
    constrainPitch();
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
    orbitYaw = yawAngle;
    orbitPitch = pitchAngle;
    
    constrainPitch();
    updateOrbitCamera();
}

void Camera::handleEditorMouseInput(double mouseX, double mouseY, bool leftPressed, bool rightPressed, bool middlePressed, bool altPressed) {
    if (!altPressed) return;
    
    if (firstMouse) {
        lastMouseX = static_cast<float>(mouseX);
        lastMouseY = static_cast<float>(mouseY);
        firstMouse = false;
        return;
    }
    
    float xOffset = static_cast<float>(mouseX) - lastMouseX;
    float yOffset = lastMouseY - static_cast<float>(mouseY);
    
    lastMouseX = static_cast<float>(mouseX);
    lastMouseY = static_cast<float>(mouseY);
    
    if (altPressed && leftPressed) {
        // Orbit camera
        orbitYaw += xOffset * mouseSensitivity;
        orbitPitch += yOffset * mouseSensitivity;
        constrainPitch();
        updateOrbitCamera();
    } else if (altPressed && rightPressed) {
        // Zoom camera
        orbitDistance -= yOffset * 0.1f;
        orbitDistance = std::max(0.1f, orbitDistance);
        updateOrbitCamera();
    } else if (altPressed && middlePressed) {
        // Pan camera
        glm::vec3 panOffset = (right * -xOffset + up * yOffset) * 0.01f * orbitDistance;
        orbitCenter += panOffset;
        updateOrbitCamera();
    }
}

void Camera::handleEditorScroll(double yOffset) {
    if (cameraMode == CameraMode::Editor) {
        orbitDistance -= yOffset * 0.5f;
        orbitDistance = std::max(0.1f, orbitDistance);
        updateOrbitCamera();
    }
}

void Camera::handleGameMouseInput(double mouseX, double mouseY, bool rightPressed) {
    // In UE5-style, mouse look works when right button is held
    if (!rightPressed) {
        // Reset first mouse flag when not looking
        firstMouse = true;
        return;
    }
    
    if (firstMouse) {
        lastMouseX = static_cast<float>(mouseX);
        lastMouseY = static_cast<float>(mouseY);
        firstMouse = false;
        return;
    }
    
    float xOffset = static_cast<float>(mouseX) - lastMouseX;
    float yOffset = lastMouseY - static_cast<float>(mouseY);
    
    lastMouseX = static_cast<float>(mouseX);
    lastMouseY = static_cast<float>(mouseY);
    
    // Only rotate if there's actual movement
    if (std::abs(xOffset) > 0.1f || std::abs(yOffset) > 0.1f) {
        rotate(xOffset, yOffset);
    }
}

void Camera::handleGameKeyboardInput(float deltaTime) {
    targetVelocity = glm::vec3(0.0f);
    
    float speedMultiplier = 1.0f;
    if (IsKeyHeld(KeyCode::Shift)) {
        speedMultiplier = 2.0f; // Faster movement with Shift
    }
    
    if (IsKeyHeld(KeyCode::W)) {
        targetVelocity += front * movementSpeed * speedMultiplier;
    }
    if (IsKeyHeld(KeyCode::S)) {
        targetVelocity -= front * movementSpeed * speedMultiplier;
    }
    if (IsKeyHeld(KeyCode::A)) {
        targetVelocity -= right * movementSpeed * speedMultiplier;
    }
    if (IsKeyHeld(KeyCode::D)) {
        targetVelocity += right * movementSpeed * speedMultiplier;
    }
    if (IsKeyHeld(KeyCode::Q)) {
        targetVelocity -= up * movementSpeed * speedMultiplier;
    }
    if (IsKeyHeld(KeyCode::E)) {
        targetVelocity += up * movementSpeed * speedMultiplier;
    }
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

void Camera::setAcceleration(float accel) {
    acceleration = accel;
}

void Camera::setDeceleration(float decel) {
    deceleration = decel;
}

void Camera::update(float deltaTime) {
    // Handle input based on camera mode
    switch (cameraMode) {
        case CameraMode::Game:
            if (keyboardMovementEnabled) {
                handleGameKeyboardInput(deltaTime);
            }
            if (mouseLookEnabled) {
                double mouseX, mouseY;
                InputManager::getInstance().getMousePosition(mouseX, mouseY);
                bool rightPressed = IsMouseButtonHeld(static_cast<int>(KeyCode::MouseRight));
                // Debug logging for right mouse button
                static int debugCounter = 0;
                if (debugCounter++ % 30 == 0) {
                    VF_LOG_INFO("Right mouse held: {}", rightPressed ? "true" : "false");
                }
                handleGameMouseInput(mouseX, mouseY, rightPressed);
            }
            break;
            
        case CameraMode::Editor:
            {
                double mouseX, mouseY;
                InputManager::getInstance().getMousePosition(mouseX, mouseY);
                bool leftPressed = IsMouseButtonHeld(static_cast<int>(KeyCode::MouseLeft));
                bool rightPressed = IsMouseButtonHeld(static_cast<int>(KeyCode::MouseRight));
                bool middlePressed = IsMouseButtonHeld(static_cast<int>(KeyCode::MouseMiddle));
                bool altPressed = IsKeyHeld(KeyCode::Alt);
                handleEditorMouseInput(mouseX, mouseY, leftPressed, rightPressed, middlePressed, altPressed);
            }
            break;
            
        case CameraMode::Cinematic:
            // Smooth camera movements for cinematic mode
            break;
    }
    
    // Apply movement
    applyMovementAcceleration(deltaTime);
    
    // Update orbit camera if needed
    if (orbitMode) {
        updateOrbitCamera();
    }
}

void Camera::applyMovementAcceleration(float deltaTime) {
    // For Game and Editor modes, movement is instant (no acceleration)
    if (cameraMode == CameraMode::Game || cameraMode == CameraMode::Editor) {
        velocity = targetVelocity;
    } else {
        // Cinematic mode: smooth acceleration/deceleration
        if (glm::length(targetVelocity) > 0.0f) {
            velocity = glm::mix(velocity, targetVelocity, std::min(1.0f, acceleration * deltaTime));
        } else {
            velocity = glm::mix(velocity, glm::vec3(0.0f), std::min(1.0f, deceleration * deltaTime));
        }
    }
    // Apply velocity to position
    if (glm::length(velocity) > 0.001f) {
        position += velocity * deltaTime;
        viewMatrixDirty = true;
        frustumValid = false;
    }
}

void Camera::updateOrbitCamera() {
    // Calculate position from spherical coordinates
    float x = orbitDistance * cos(glm::radians(orbitYaw)) * cos(glm::radians(orbitPitch));
    float y = orbitDistance * sin(glm::radians(orbitPitch));
    float z = orbitDistance * sin(glm::radians(orbitYaw)) * cos(glm::radians(orbitPitch));
    
    position = orbitCenter + glm::vec3(x, y, z);
    target = orbitCenter;
    
    // Update camera vectors
    front = glm::normalize(target - position);
    right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front));
    
    // Update yaw and pitch for consistency
    yaw = orbitYaw;
    pitch = orbitPitch;
    
    viewMatrixDirty = true;
    frustumValid = false;
}

void Camera::updateVectors() {
    // Calculate the new front vector
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    
    // Recalculate the right and up vector
    right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::updateProjectionMatrix() {
    if (type == CameraType::Perspective) {
        // Use horizontal FOV like UE5
        float verticalFOV = 2.0f * atan(tan(glm::radians(fov) * 0.5f) / aspectRatio);
        projectionMatrix = glm::perspective(verticalFOV, aspectRatio, nearPlane, farPlane);
    } else {
        float halfSize = orthographicSize * 0.5f;
        projectionMatrix = glm::ortho(-halfSize * aspectRatio, halfSize * aspectRatio, 
                                     -halfSize, halfSize, nearPlane, farPlane);
    }
    projectionMatrixDirty = false;
}

void Camera::constrainPitch() {
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
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
        const_cast<Camera*>(this)->updateProjectionMatrix();
    }
    return projectionMatrix;
}

glm::mat4 Camera::getViewProjectionMatrix() const {
    return getProjectionMatrix() * getViewMatrix();
}

bool Camera::isPointInFrustum(const glm::vec3& point) const {
    calculateFrustumPlanes();
    
    for (const auto& plane : frustumPlanes) {
        if (glm::dot(plane.normal, point) + plane.distance < 0) {
            return false;
        }
    }
    return true;
}

bool Camera::isSphereInFrustum(const glm::vec3& center, float radius) const {
    calculateFrustumPlanes();
    
    for (const auto& plane : frustumPlanes) {
        if (glm::dot(plane.normal, center) + plane.distance < -radius) {
            return false;
        }
    }
    return true;
}

bool Camera::isBoxInFrustum(const glm::vec3& min, const glm::vec3& max) const {
    calculateFrustumPlanes();
    
    for (const auto& plane : frustumPlanes) {
        glm::vec3 positive = max;
        glm::vec3 negative = min;
        
        if (plane.normal.x < 0) {
            positive.x = min.x;
            negative.x = max.x;
        }
        if (plane.normal.y < 0) {
            positive.y = min.y;
            negative.y = max.y;
        }
        if (plane.normal.z < 0) {
            positive.z = min.z;
            negative.z = max.z;
        }
        
        if (glm::dot(plane.normal, positive) + plane.distance < 0) {
            return false;
        }
    }
    return true;
}

void Camera::calculateFrustumPlanes() const {
    if (frustumValid) return;
    
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
    
    // Normalize planes
    for (auto& plane : frustumPlanes) {
        float length = glm::length(plane.normal);
        plane.normal /= length;
        plane.distance /= length;
    }
    
    const_cast<Camera*>(this)->frustumValid = true;
}

void Camera::bindInputControls(InputManager& inputManager) {
    // Input controls are handled in update() method
    VF_LOG_DEBUG("Camera input controls bound");
}

void Camera::unbindInputControls() {
    // Clear any input bindings if needed
    inputBindings.clear();
    VF_LOG_DEBUG("Camera input controls unbound");
}

// CameraController Implementation
CameraController::CameraController(std::shared_ptr<Camera> cam)
    : camera(cam) {
    VF_LOG_DEBUG("CameraController created");
}

void CameraController::update(float deltaTime) {
    if (camera) {
        camera->update(deltaTime);
    }
}

void CameraController::setGameMode() {
    if (camera) {
        camera->setCameraMode(CameraMode::Game);
        camera->enableMouseLook(true);
        camera->enableKeyboardMovement(true);
    }
}

void CameraController::setEditorMode() {
    if (camera) {
        camera->setCameraMode(CameraMode::Editor);
        camera->enableMouseLook(false);
        camera->enableKeyboardMovement(false);
    }
}

void CameraController::setCinematicMode() {
    if (camera) {
        camera->setCameraMode(CameraMode::Cinematic);
        camera->enableMouseLook(false);
        camera->enableKeyboardMovement(false);
    }
}

void CameraController::handleMouseMovement(double xOffset, double yOffset) {
    if (camera) {
        camera->rotate(static_cast<float>(xOffset), static_cast<float>(yOffset));
    }
}

void CameraController::handleMouseScroll(double yOffset) {
    if (camera) {
        camera->handleEditorScroll(yOffset);
    }
}

void CameraController::handleKeyboardInput(CameraMovement direction, bool pressed) {
    if (camera && pressed) {
        camera->move(direction, 1.0f);
    }
}

} // namespace Core
} // namespace VaporFrame 