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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan uses 0.0 to 1.0 depth range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // For glm::translate, glm::rotate, glm::scale

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

    // All Vulkan specific members are GONE from here.

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    void initWindow() {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW!");
        }
        std::cout << "GLFW initialized successfully." << std::endl;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, "VaporFrame Engine - Vulkan", nullptr, nullptr);
        if (!window) {
            throw std::runtime_error("Failed to create GLFW window!");
        }
        glfwSetWindowUserPointer(window, this); 
        glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
        std::cout << "GLFW window created successfully." << std::endl;
    }

    void initVulkan() {
        vulkanRenderer = new VulkanRenderer(window, validationLayers, enableValidationLayers); // enableValidationLayers is a global const here
        vulkanRenderer->initVulkan();
        std::cout << "HelloVulkanApp: Vulkan initialization delegated to VulkanRenderer." << std::endl;
    }

    void drawFrame() {
        if (vulkanRenderer) {
            vulkanRenderer->drawFrame();
        }
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
    }

    void cleanup() {
        std::cout << "Starting cleanup in HelloVulkanApp..." << std::endl;
        
        if (vulkanRenderer) {
            vulkanRenderer->cleanup(); 
            delete vulkanRenderer;
            vulkanRenderer = nullptr;
            std::cout << "VulkanRenderer cleaned up and deleted." << std::endl;
        }

        if (window != nullptr) { 
            glfwDestroyWindow(window);
            window = nullptr; 
            std::cout << "GLFW window destroyed." << std::endl;
        }
        glfwTerminate();
        std::cout << "GLFW terminated." << std::endl;
    }
};

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
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
        std::cerr << "Unhandled Exception caught in main: " << e.what() << std::endl;
        std::cerr << "Press Enter to exit..." << std::endl;
        std::cin.get(); 
        return EXIT_FAILURE;
    }

    std::cout << "Application finished successfully. Press Enter to exit..." << std::endl;
    std::cin.get(); 
    return EXIT_SUCCESS;
} 