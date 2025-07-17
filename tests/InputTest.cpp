#include "../src/Core/InputManager.h"
#include "../src/Core/Logger.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <thread>

using namespace VaporFrame::Core;

// Test callbacks
bool escapePressed = false;
bool spacePressed = false;
bool mouseLeftPressed = false;
int keyPressCount = 0;
int mousePressCount = 0;

void onEscapePressed() {
    escapePressed = true;
    keyPressCount++;
    VF_LOG_INFO("Escape key pressed!");
}

void onSpacePressed() {
    spacePressed = true;
    keyPressCount++;
    VF_LOG_INFO("Space key pressed!");
}

void onMouseLeftPressed() {
    mouseLeftPressed = true;
    mousePressCount++;
    VF_LOG_INFO("Left mouse button pressed!");
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        VF_LOG_ERROR("Failed to initialize GLFW");
        return -1;
    }
    
    // Create a hidden window for input testing
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Input Test", nullptr, nullptr);
    if (!window) {
        VF_LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }
    
    // Initialize InputManager
    InputManager& inputManager = InputManager::getInstance();
    inputManager.initialize(window);
    
    // Bind test actions
    inputManager.bindAction("Escape", InputDevice::Keyboard, static_cast<int>(KeyCode::Escape), 
                           InputAction::Press, onEscapePressed);
    inputManager.bindAction("Space", InputDevice::Keyboard, static_cast<int>(KeyCode::Space), 
                           InputAction::Press, onSpacePressed);
    inputManager.bindAction("MouseLeft", InputDevice::Mouse, static_cast<int>(KeyCode::MouseLeft), 
                           InputAction::Press, onMouseLeftPressed);
    
    VF_LOG_INFO("Input test started. Press keys and mouse buttons to test.");
    VF_LOG_INFO("Press ESC to exit the test.");
    
    // Test loop
    auto startTime = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        inputManager.update();
        
        // Check for escape to exit
        if (escapePressed) {
            VF_LOG_INFO("Escape detected, exiting test.");
            break;
        }
        
        // Test direct input queries
        if (inputManager.isKeyPressed(KeyCode::W)) {
            VF_LOG_INFO("W key is pressed");
        }
        
        if (inputManager.isKeyHeld(KeyCode::A)) {
            VF_LOG_INFO("A key is held");
        }
        
        if (inputManager.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
            VF_LOG_INFO("Right mouse button pressed");
        }
        
        // Test mouse position and movement
        double mouseX, mouseY;
        inputManager.getMousePosition(mouseX, mouseY);
        
        double deltaX, deltaY;
        inputManager.getMouseDelta(deltaX, deltaY);
        
        if (std::abs(deltaX) > 0.1 || std::abs(deltaY) > 0.1) {
            VF_LOG_INFO("Mouse moved: delta({:.2f}, {:.2f})", deltaX, deltaY);
        }
        
        // Test mouse scroll
        double scrollX, scrollY;
        inputManager.getMouseScroll(scrollX, scrollY);
        
        if (std::abs(scrollX) > 0.1 || std::abs(scrollY) > 0.1) {
            VF_LOG_INFO("Mouse scroll: ({:.2f}, {:.2f})", scrollX, scrollY);
        }
        
        // Test gamepad
        if (inputManager.isGamepadConnected()) {
            VF_LOG_INFO("Gamepad connected!");
            
            // Test left stick
            float leftX = inputManager.getGamepadAxis(GLFW_JOYSTICK_1, 0);
            float leftY = inputManager.getGamepadAxis(GLFW_JOYSTICK_1, 1);
            
            if (std::abs(leftX) > 0.1 || std::abs(leftY) > 0.1) {
                VF_LOG_INFO("Left stick: ({:.2f}, {:.2f})", leftX, leftY);
            }
            
            // Test buttons
            if (inputManager.isGamepadButtonPressed(GLFW_JOYSTICK_1, 0)) {
                VF_LOG_INFO("Gamepad button 0 pressed");
            }
        }
        
        // Check test duration
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);
        
        if (duration.count() >= 30) { // 30 second timeout
            VF_LOG_INFO("Test timeout reached (30 seconds)");
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    // Test results
    VF_LOG_INFO("=== Input Test Results ===");
    VF_LOG_INFO("Key press count: {}", keyPressCount);
    VF_LOG_INFO("Mouse press count: {}", mousePressCount);
    VF_LOG_INFO("Escape pressed: {}", escapePressed ? "Yes" : "No");
    VF_LOG_INFO("Space pressed: {}", spacePressed ? "Yes" : "No");
    VF_LOG_INFO("Mouse left pressed: {}", mouseLeftPressed ? "Yes" : "No");
    
    // Test input state reset
    VF_LOG_INFO("Testing input state reset...");
    inputManager.resetState();
    
    // Test convenience functions
    VF_LOG_INFO("Testing convenience functions...");
    bool wPressed = IsKeyPressed(KeyCode::W);
    bool aHeld = IsKeyHeld(KeyCode::A);
    bool mouseRightPressed = IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
    
    VF_LOG_INFO("W pressed: {}, A held: {}, Mouse right: {}", 
                wPressed, aHeld, mouseRightPressed);
    
    // Cleanup
    inputManager.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    
    VF_LOG_INFO("Input test completed successfully!");
    
    // Keep console open
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    
    return 0;
} 