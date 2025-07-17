#define GLM_ENABLE_EXPERIMENTAL
#include "../src/Core/Camera.h"
#include "../src/Core/InputManager.h"
#include "../src/Core/Logger.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <thread>

using namespace VaporFrame::Core;

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        VF_LOG_ERROR("Failed to initialize GLFW");
        return -1;
    }
    
    // Create a hidden window for testing
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Camera Test", nullptr, nullptr);
    if (!window) {
        VF_LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }
    
    // Initialize InputManager
    InputManager& inputManager = InputManager::getInstance();
    inputManager.initialize(window);
    
    // Create camera
    auto camera = std::make_shared<Camera>(CameraType::Perspective);
    camera->setPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    camera->setAspectRatio(800.0f / 600.0f);
    camera->setFOV(45.0f);
    camera->setNearPlane(0.1f);
    camera->setFarPlane(100.0f);
    
    // Create camera controller
    CameraController controller(camera);
    
    // Enable camera controls - UE5 compliant
    camera->setCameraMode(CameraMode::Game);
    camera->enableMouseLook(true);
    camera->enableKeyboardMovement(true);
    camera->bindInputControls(inputManager);
    
    VF_LOG_INFO("UE5 Camera test started");
    VF_LOG_INFO("Controls: WASD to move, Right-click to look, ESC to exit");
    VF_LOG_INFO("Press F1-F3 to switch camera modes (Game/Editor/Cinematic)");
    
    // Test loop
    auto startTime = std::chrono::high_resolution_clock::now();
    auto lastTime = startTime;
    
    while (!glfwWindowShouldClose(window)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        glfwPollEvents();
        inputManager.update();
        
        // Check for exit
        if (IsKeyPressed(KeyCode::Escape)) {
            VF_LOG_INFO("Escape pressed, exiting test");
            break;
        }
        
        // Camera mode switching - UE5 style
        if (IsKeyPressed(KeyCode::F1)) {
            camera->setCameraMode(CameraMode::Game);
            VF_LOG_INFO("Switched to Game mode");
        }
        if (IsKeyPressed(KeyCode::F2)) {
            camera->setCameraMode(CameraMode::Editor);
            VF_LOG_INFO("Switched to Editor mode");
        }
        if (IsKeyPressed(KeyCode::F3)) {
            camera->setCameraMode(CameraMode::Cinematic);
            VF_LOG_INFO("Switched to Cinematic mode");
        }
        
        // Update camera
        camera->update(deltaTime);
        controller.update(deltaTime);
        
        // Log camera state every 2 seconds
        static float logTimer = 0.0f;
        logTimer += deltaTime;
        if (logTimer >= 2.0f) {
            glm::vec3 pos = camera->getPosition();
            glm::vec3 front = camera->getFront();
            glm::vec3 up = camera->getUp();
            float fov = camera->getFOV();
            
            VF_LOG_INFO("Camera State:");
            VF_LOG_INFO("  Position: ({:.2f}, {:.2f}, {:.2f})", pos.x, pos.y, pos.z);
            VF_LOG_INFO("  Front: ({:.2f}, {:.2f}, {:.2f})", front.x, front.y, front.z);
            VF_LOG_INFO("  Up: ({:.2f}, {:.2f}, {:.2f})", up.x, up.y, up.z);
            VF_LOG_INFO("  FOV: {:.2f}", fov);
            
            logTimer = 0.0f;
        }
        
        // Test matrix calculations
        glm::mat4 viewMatrix = camera->getViewMatrix();
        glm::mat4 projMatrix = camera->getProjectionMatrix();
        glm::mat4 vpMatrix = camera->getViewProjectionMatrix();
        
        // Test frustum culling
        glm::vec3 testPoint(0.0f, 0.0f, 0.0f);
        bool inFrustum = camera->isPointInFrustum(testPoint);
        
        // Test sphere culling
        bool sphereInFrustum = camera->isSphereInFrustum(glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
        
        // Test box culling
        bool boxInFrustum = camera->isBoxInFrustum(
            glm::vec3(-1.0f, -1.0f, -1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
        
        // Check test duration
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);
        if (duration.count() >= 30) { // 30 second timeout
            VF_LOG_INFO("Test timeout reached (30 seconds)");
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    // Test results
    VF_LOG_INFO("=== Camera Test Results ===");
    VF_LOG_INFO("Camera position: ({:.2f}, {:.2f}, {:.2f})", 
                camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);
    VF_LOG_INFO("Camera FOV: {:.2f}", camera->getFOV());
    VF_LOG_INFO("Camera type: {}", camera->getType() == CameraType::Perspective ? "Perspective" : "Orthographic");
    
    // Test matrix properties
    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 proj = camera->getProjectionMatrix();
    
    VF_LOG_INFO("View matrix determinant: {:.6f}", glm::determinant(view));
    VF_LOG_INFO("Projection matrix determinant: {:.6f}", glm::determinant(proj));
    
    // Test orthographic camera
    VF_LOG_INFO("Testing orthographic camera...");
    auto orthoCamera = std::make_shared<Camera>(CameraType::Orthographic);
    orthoCamera->setAspectRatio(800.0f / 600.0f);
    orthoCamera->setOrthographicSize(10.0f);
    orthoCamera->setNearPlane(0.1f);
    orthoCamera->setFarPlane(100.0f);
    
    glm::mat4 orthoProj = orthoCamera->getProjectionMatrix();
    VF_LOG_INFO("Orthographic projection matrix determinant: {:.6f}", glm::determinant(orthoProj));
    
    // Cleanup
    camera->unbindInputControls();
    inputManager.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    
    VF_LOG_INFO("Camera test completed successfully!");
    
    // Keep console open
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    
    return 0;
} 