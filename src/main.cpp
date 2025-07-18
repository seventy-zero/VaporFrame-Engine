#define GLFW_INCLUDE_VULKAN // Important: Tells GLFW to include Vulkan headers and provide Vulkan-specific functions
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <stdexcept> // For std::runtime_error
#include <cstring> // For strcmp
#include <optional> // For QueueFamilyIndices
#include <set> // For checking unique queue families
#include <algorithm> // For std::min, std::max, std::clamp
#include <fstream> // For file reading
#include <string> // For std::string manipulations
#include <array> // For std::array
#ifdef _WIN32
#include <windows.h> // For GetCurrentDirectoryA
#include <libloaderapi.h> // For GetModuleFileNameA
#endif

#include "VulkanRenderer.h" // Include the new renderer header
#include "Core/Logger.h"
#include "Core/MemoryManager.h"
#include "Core/InputManager.h"
#include "Core/Camera.h"
#include "Core/SceneGraph.h" // [1] Include SceneGraph
#include "Core/UISystem.h" // [2] Include UISystem
#include "Core/ImGuiUI.h"
#include "Core/WebViewUI.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan uses 0.0 to 1.0 depth range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // For glm::translate, glm::rotate, glm::scale

using namespace VaporFrame::Core;

// Configuration
const bool enableValidationLayers = true; // This is passed to VulkanRenderer constructor

// Validation Layers (this global is still used to pass to VulkanRenderer constructor)
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// Forward declarations for static callback functions
static void glfw_error_callback(int error, const char* description);
static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);

// Helper function to load vkCreateDebugUtilsMessengerEXT
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Helper function to load vkDestroyDebugUtilsMessengerEXT
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class HelloVulkanApp {
public:
    void run() {
        initSystems();
        initWindow();
        initVulkan(); 
        mainLoop();
        cleanup();
    }

    void setRendererFramebufferResized(bool resized) {
        if (vulkanRenderer) {
            vulkanRenderer->framebufferResized = resized;
        }
    }

private:
    GLFWwindow* window;
    VulkanRenderer* vulkanRenderer; // Pointer to our renderer
    std::shared_ptr<Camera> camera;
    float lastFrameTime = 0.0f;
    std::unique_ptr<UISystem> uiSystem;

    // Scene Graph/ECS system
    SceneManager& sceneManager = SceneManager::getInstance(); // [2] SceneManager singleton
    Scene* mainScene = nullptr; // [2] Main scene pointer

    // All Vulkan specific members are GONE from here.

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    void initSystems() {
        // Initialize logger
        VaporFrame::Logger::getInstance().initialize("vaporframe.log");
        VF_LOG_INFO("Starting VaporFrame Engine");
        
        // Initialize memory manager
        MemoryManager::getInstance().initialize();
        VF_LOG_INFO("Memory manager initialized successfully");
        
        // [3] Initialize SceneManager and create main scene
        mainScene = sceneManager.createScene("MainScene");
        sceneManager.setActiveScene(mainScene);
        VF_LOG_INFO("SceneManager and main scene initialized");
    }

    void initWindow() {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW!");
        }
        VF_LOG_INFO("GLFW initialized successfully");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, "VaporFrame Engine - Vulkan", nullptr, nullptr);
        if (!window) {
            throw std::runtime_error("Failed to create GLFW window!");
        }
        glfwSetWindowUserPointer(window, this); 
        glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
        
        // Initialize InputManager
        InputManager::getInstance().initialize(window);
        VF_LOG_INFO("InputManager initialized successfully");
        
        VF_LOG_INFO("GLFW window created successfully");
    }

    void initVulkan() {
        vulkanRenderer = new VulkanRenderer(window, validationLayers, enableValidationLayers); // enableValidationLayers is a global const here
        vulkanRenderer->initVulkan();
        VF_LOG_INFO("Vulkan initialization delegated to VulkanRenderer");

        // Camera setup - UE5 compliant
        camera = std::make_shared<Camera>(CameraType::Perspective);
        camera->setPosition(glm::vec3(2.0f, 2.0f, 2.0f));
        camera->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
        camera->setUp(glm::vec3(0.0f, 1.0f, 0.0f)); // UE5 uses Y-up
        camera->setAspectRatio(static_cast<float>(WIDTH) / static_cast<float>(HEIGHT));
        camera->setFOV(90.0f); // UE5 default horizontal FOV
        camera->setNearPlane(0.1f);
        camera->setFarPlane(100.0f);
        camera->setCameraMode(CameraMode::Game); // Start in game mode
        camera->enableMouseLook(true);
        camera->enableKeyboardMovement(true);
        camera->setMouseSensitivity(0.2f);  // More sensitive mouse
        camera->setMovementSpeed(8.0f);     // Faster movement
        camera->setAcceleration(50.0f);     // Much faster acceleration
        camera->setDeceleration(20.0f);     // Faster deceleration
        camera->bindInputControls(InputManager::getInstance());
        VF_LOG_INFO("UE5-compliant camera initialized and input controls bound");

        // [4] Create test entities/components in the scene
        if (mainScene) {
            // Camera entity
            SceneNode* ecsCamera = mainScene->createEntity("ECS_Camera");
            auto* camComp = ecsCamera->addComponent<CameraComponent>();
            camComp->fov = 90.0f;
            camComp->nearPlane = 0.1f;
            camComp->farPlane = 100.0f;
            camComp->isMainCamera = true;
            ecsCamera->getTransform()->setPosition(glm::vec3(2.0f, 2.0f, 2.0f));
            
            // Test mesh loading with generated geometry
            SceneNode* cubeEntity = mainScene->createEntity("ECS_Cube");
            auto* cubeComp = cubeEntity->addComponent<MeshComponent>();
            cubeComp->setMesh(MeshUtils::createCube(1.0f));
            cubeComp->visible = true;
            cubeEntity->getTransform()->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
            
            // Sphere entity
            SceneNode* sphereEntity = mainScene->createEntity("ECS_Sphere");
            auto* sphereComp = sphereEntity->addComponent<MeshComponent>();
            sphereComp->setMesh(MeshUtils::createSphere(0.5f, 16));
            sphereComp->visible = true;
            sphereEntity->getTransform()->setPosition(glm::vec3(2.0f, 0.0f, 0.0f));
            
            // Plane entity
            SceneNode* planeEntity = mainScene->createEntity("ECS_Plane");
            auto* planeComp = planeEntity->addComponent<MeshComponent>();
            planeComp->setMesh(MeshUtils::createPlane(5.0f, 5.0f, 1));
            planeComp->visible = true;
            planeEntity->getTransform()->setPosition(glm::vec3(0.0f, -1.0f, 0.0f));
            
            // Light entity
            SceneNode* lightEntity = mainScene->createEntity("ECS_Light");
            auto* lightComp = lightEntity->addComponent<LightComponent>();
            lightComp->type = LightComponent::LightType::Point;
            lightComp->color = glm::vec3(1.0f, 0.9f, 0.7f);
            lightComp->intensity = 2.0f;
            lightEntity->getTransform()->setPosition(glm::vec3(1.0f, 3.0f, 1.0f));
            
            // Add a child entity to cube
            SceneNode* cubeChild = mainScene->createChildEntity(cubeEntity, "ECS_CubeChild");
            cubeChild->getTransform()->setPosition(glm::vec3(0.0f, 1.5f, 0.0f));
            auto* childComp = cubeChild->addComponent<MeshComponent>();
            childComp->setMesh(MeshUtils::createSphere(0.3f, 8));
            childComp->visible = true;
            
            VF_LOG_INFO("Test ECS entities with mesh loading created in main scene");
        }
        
        // Initialize UI System
        uiSystem = std::make_unique<UISystem>();
        if (!uiSystem->initialize()) {
            VF_LOG_ERROR("Failed to initialize UI System");
            return;
        }
        
        // Create ImGui UI for debug panels
        auto imGuiUI = uiSystem->createImGuiUI("DebugUI");
        if (imGuiUI) {
            imGuiUI->setSceneManager(&sceneManager);
            imGuiUI->setInputManager(&InputManager::getInstance());
            imGuiUI->setMemoryManager(&MemoryManager::getInstance());
            imGuiUI->setCamera(camera.get());
        }
        
        // Create WebView UI for main menu
        auto webViewUI = uiSystem->createWebViewUI("MainMenu", "assets/ui/pages/main-menu.html");
        if (webViewUI) {
            // Register callbacks for WebView communication
            webViewUI->registerCallback("startNewGame", [this](const std::string& data) {
                VF_LOG_INFO("WebView: Start new game requested");
                // TODO: Implement new game logic
            });
            
            webViewUI->registerCallback("loadGame", [this](const std::string& data) {
                VF_LOG_INFO("WebView: Load game requested");
                // TODO: Implement load game logic
            });
            
            webViewUI->registerCallback("openEditor", [this](const std::string& data) {
                VF_LOG_INFO("WebView: Open editor requested");
                // TODO: Switch to editor mode
            });
            
            webViewUI->registerCallback("openConsole", [this, imGuiUI](const std::string& data) {
                VF_LOG_INFO("WebView: Open console requested");
                if (imGuiUI) {
                    imGuiUI->setConsoleVisible(true);
                }
            });
            
            webViewUI->registerCallback("getStats", [this, webViewUI](const std::string& data) {
                // Send performance stats to WebView
                auto& memoryManager = MemoryManager::getInstance();
                auto stats = memoryManager.getGlobalStats();
                
                // Create stats JSON
                std::string statsJson = "{";
                statsJson += "\"fps\":" + std::to_string(60.0f); // TODO: Get actual FPS
                statsJson += ",\"memoryUsage\":" + std::to_string(stats.currentUsage);
                statsJson += ",\"renderTime\":" + std::to_string(16.7f); // TODO: Get actual render time
                statsJson += ",\"drawCalls\":" + std::to_string(1234); // TODO: Get actual draw calls
                statsJson += "}";
                
                webViewUI->callJavaScriptFunction("updateStats", statsJson);
            });
            
            webViewUI->registerCallback("exitEngine", [this](const std::string& data) {
                VF_LOG_INFO("WebView: Exit engine requested");
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            });
            
            // Set WebView position and size (full screen for main menu)
            webViewUI->setPosition(0, 0);
            webViewUI->setSize(WIDTH, HEIGHT);
            webViewUI->setVisible(true);
        }
        
        VF_LOG_INFO("UI System initialized with debug panels and WebView main menu");
    }

    void drawFrame() {
        if (vulkanRenderer) {
            vulkanRenderer->drawFrame();
        }
    }

    void mainLoop() {
        VF_LOG_INFO("Starting main loop");
        int frameCount = 0;
        
        while (!glfwWindowShouldClose(window)) {
            frameCount++;
            if (frameCount % 60 == 0) { // Log every 60 frames (1 second at 60fps)
                VF_LOG_INFO("Main loop iteration: {}", frameCount);
            }
            
            glfwPollEvents();
            
            // Update input manager
            InputManager::getInstance().update();
            
            // Handle input
            if (IsKeyPressed(KeyCode::Escape)) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                VF_LOG_INFO("Escape key pressed, closing application");
            }
            
            // Camera mode switching (UE5-style)
            if (IsKeyPressed(KeyCode::F1)) {
                camera->setCameraMode(CameraMode::Game);
                VF_LOG_INFO("Switched to Game camera mode");
            }
            if (IsKeyPressed(KeyCode::F2)) {
                camera->setCameraMode(CameraMode::Editor);
                VF_LOG_INFO("Switched to Editor camera mode");
            }
            if (IsKeyPressed(KeyCode::F3)) {
                camera->setCameraMode(CameraMode::Cinematic);
                VF_LOG_INFO("Switched to Cinematic camera mode");
            }
            
            // Calculate delta time
            float currentFrameTime = static_cast<float>(glfwGetTime());
            float deltaTime = currentFrameTime - lastFrameTime;
            lastFrameTime = currentFrameTime;

            // Update camera
            if (camera) {
                camera->update(deltaTime);
                // Update aspect ratio if window resized
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);
                if (height > 0) {
                    camera->setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
                }
                // Pass camera matrices to renderer
                vulkanRenderer->setViewMatrix(camera->getViewMatrix());
                vulkanRenderer->setProjectionMatrix(camera->getProjectionMatrix());
            }

            // [5] Update and render the scene
            if (frameCount == 1) {
                VF_LOG_INFO("First frame: Updating and rendering scene");
            }
            sceneManager.update(deltaTime);
            sceneManager.render();

            // Update UI System
            if (uiSystem) {
                uiSystem->update(deltaTime);
            }

            if (frameCount == 1) {
                VF_LOG_INFO("First frame: Calling drawFrame");
            }
            drawFrame();
            
            // Render UI System (simple rendering for now)
            if (uiSystem) {
                uiSystem->renderSimple();
            }
        }
        
        VF_LOG_INFO("Main loop ended after {} frames", frameCount);
    }

    void cleanup() {
        VF_LOG_INFO("Starting cleanup in HelloVulkanApp");
        
        // Shutdown InputManager
        InputManager::getInstance().shutdown();
        VF_LOG_INFO("InputManager shutdown");
        
        if (vulkanRenderer) {
            vulkanRenderer->cleanup(); 
            delete vulkanRenderer;
            vulkanRenderer = nullptr;
            VF_LOG_INFO("VulkanRenderer cleaned up and deleted");
        }

        if (window != nullptr) { 
            glfwDestroyWindow(window);
            window = nullptr; 
            VF_LOG_INFO("GLFW window destroyed");
        }
        glfwTerminate();
        VF_LOG_INFO("GLFW terminated");
        
        // Shutdown UI System
        if (uiSystem) {
            uiSystem->shutdown();
            uiSystem.reset();
        }
        
        // Shutdown systems
        MemoryManager::getInstance().shutdown();
        VaporFrame::Logger::getInstance().shutdown();
    }
};

static void glfw_error_callback(int error, const char* description) {
    VF_LOG_ERROR("GLFW Error ({}): {}", error, description);
}

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloVulkanApp*>(glfwGetWindowUserPointer(window));
    if (app) { 
        app->setRendererFramebufferResized(true);
    }
}

int main() {
    HelloVulkanApp app;

    try {
        app.run();
    } catch (const std::exception& e) {
        VF_LOG_CRITICAL("Unhandled Exception caught in main: {}", e.what());
        std::cerr << "Press Enter to exit..." << std::endl;
        std::cin.get(); 
        return EXIT_FAILURE;
    }

    VF_LOG_INFO("Application finished successfully");
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get(); 
    return EXIT_SUCCESS;
} 