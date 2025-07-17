#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <memory>
#include <vector>
#include <string>
#include <array>
#include <unordered_map>

namespace VaporFrame {
namespace Core {

// Forward declaration
class InputManager;

enum class CameraType {
    Perspective,
    Orthographic
};

enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down
};

class Camera {
public:
    // Constructor
    Camera(CameraType type = CameraType::Perspective);
    
    // Destructor
    ~Camera() = default;
    
    // Camera setup
    void setPosition(const glm::vec3& position);
    void setTarget(const glm::vec3& target);
    void setUp(const glm::vec3& up);
    void setFOV(float fov);
    void setAspectRatio(float aspectRatio);
    void setNearPlane(float nearPlane);
    void setFarPlane(float farPlane);
    void setOrthographicSize(float size);
    
    // Camera movement
    void move(const glm::vec3& offset);
    void move(CameraMovement direction, float distance);
    void rotate(float yaw, float pitch);
    void rotateAroundTarget(float yaw, float pitch);
    void lookAt(const glm::vec3& target);
    void orbit(const glm::vec3& center, float distance, float yaw, float pitch);
    
    // Camera controls
    void enableMouseLook(bool enable);
    void enableKeyboardMovement(bool enable);
    void setMouseSensitivity(float sensitivity);
    void setMovementSpeed(float speed);
    void setRotationSpeed(float speed);
    
    // Update camera (call once per frame)
    void update(float deltaTime);
    
    // Matrix getters
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    glm::mat4 getViewProjectionMatrix() const;
    
    // Camera state getters
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getTarget() const { return target; }
    glm::vec3 getUp() const { return up; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getRight() const { return right; }
    float getFOV() const { return fov; }
    float getAspectRatio() const { return aspectRatio; }
    float getNearPlane() const { return nearPlane; }
    float getFarPlane() const { return farPlane; }
    CameraType getType() const { return type; }
    
    // Frustum culling
    bool isPointInFrustum(const glm::vec3& point) const;
    bool isSphereInFrustum(const glm::vec3& center, float radius) const;
    bool isBoxInFrustum(const glm::vec3& min, const glm::vec3& max) const;
    
    // Camera presets
    void setFirstPersonMode();
    void setThirdPersonMode();
    void setOrbitMode();
    void setTopDownMode();
    
    // Input integration
    void bindInputControls(InputManager& inputManager);
    void unbindInputControls();
    
private:
    // Camera properties
    CameraType type;
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    glm::vec3 front;
    glm::vec3 right;
    
    // Projection properties
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    float orthographicSize;
    
    // Movement properties
    float movementSpeed;
    float rotationSpeed;
    float mouseSensitivity;
    
    // Control flags
    bool mouseLookEnabled;
    bool keyboardMovementEnabled;
    
    // Mouse look state
    bool firstMouse;
    float lastMouseX;
    float lastMouseY;
    float yaw;
    float pitch;
    
    // Orbit mode properties
    bool orbitMode;
    glm::vec3 orbitCenter;
    float orbitDistance;
    
    // Input binding IDs
    std::vector<std::string> inputBindings;
    
    // Helper functions
    void updateVectors();
    void updateProjectionMatrix();
    void constrainPitch();
    void calculateFrustumPlanes() const;
    
    // Frustum planes for culling
    struct FrustumPlane {
        glm::vec3 normal;
        float distance;
    };
    mutable std::array<FrustumPlane, 6> frustumPlanes;
    mutable bool frustumValid;
    
    // Cached matrices
    mutable glm::mat4 viewMatrix;
    mutable glm::mat4 projectionMatrix;
    mutable bool viewMatrixDirty;
    mutable bool projectionMatrixDirty;
};

// Camera controller for easy camera manipulation
class CameraController {
public:
    CameraController(std::shared_ptr<Camera> camera);
    
    // Update controller
    void update(float deltaTime);
    
    // Camera access
    std::shared_ptr<Camera> getCamera() const { return camera; }
    
    // Control modes
    void setFreeLookMode();
    void setOrbitMode(const glm::vec3& center);
    void setFollowMode(const glm::vec3& target, float distance);
    
    // Input handling
    void handleMouseMovement(double xOffset, double yOffset);
    void handleMouseScroll(double yOffset);
    void handleKeyboardInput(CameraMovement direction, bool pressed);
    
private:
    std::shared_ptr<Camera> camera;
    glm::vec3 followTarget;
    float followDistance;
    bool followMode;
    
    // Input state
    std::unordered_map<CameraMovement, bool> movementState;
};

} // namespace Core
} // namespace VaporFrame 