#include "InputManager.h"
#include "Logger.h"
#include <algorithm>
#include <iostream>

namespace VaporFrame {
namespace Core {

InputManager& InputManager::getInstance() {
    static InputManager instance;
    return instance;
}

void InputManager::initialize(GLFWwindow* window) {
    this->window = window;
    
    // Set GLFW callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetJoystickCallback(gamepadCallback);
    
    // Initialize mouse position
    glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
    mouse.x = lastMouseX;
    mouse.y = lastMouseY;
    
    // Initialize gamepad states
    for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_LAST; ++i) {
        if (glfwJoystickPresent(i)) {
            updateGamepadState(i);
        }
    }
    
    VF_LOG_INFO("InputManager initialized successfully");
}

void InputManager::shutdown() {
    clearBindings();
    window = nullptr;
    VF_LOG_INFO("InputManager shutdown");
}

void InputManager::update() {
    // Clear frame-specific state
    keyboard.pressedKeys.clear();
    keyboard.releasedKeys.clear();
    mouse.deltaX = 0.0;
    mouse.deltaY = 0.0;
    mouse.scrollX = 0.0;
    mouse.scrollY = 0.0;
    
    // Update gamepad states
    for (auto& pair : gamepads) {
        if (pair.second.connected) {
            updateGamepadState(pair.first);
        }
    }
    
    // Process input bindings
    processBindings();
    
    // Update mouse delta
    double currentX, currentY;
    glfwGetCursorPos(window, &currentX, &currentY);
    mouse.deltaX = currentX - lastMouseX;
    mouse.deltaY = currentY - lastMouseY;
    lastMouseX = currentX;
    lastMouseY = currentY;

    // --- FIX: Update mouse button held state ---
    for (auto& pair : mouse.buttonStates) {
        int button = pair.first;
        InputState& state = pair.second;
        int glfwState = glfwGetMouseButton(window, button);
        if (state == InputState::Pressed && glfwState == GLFW_PRESS) {
            state = InputState::Held;
        } else if (state == InputState::Released && glfwState == GLFW_RELEASE) {
            // Remove released buttons from state map
            state = InputState::Released;
        }
    }
    // Remove released mouse buttons from state map
    for (auto it = mouse.buttonStates.begin(); it != mouse.buttonStates.end(); ) {
        if (it->second == InputState::Released) {
            it = mouse.buttonStates.erase(it);
        } else {
            ++it;
        }
    }

    // --- FIX: Update key held state ---
    for (auto& pair : keyboard.keyStates) {
        int key = pair.first;
        InputState& state = pair.second;
        int glfwState = glfwGetKey(window, key);
        if (state == InputState::Pressed && glfwState == GLFW_PRESS) {
            state = InputState::Held;
        } else if (state == InputState::Released && glfwState == GLFW_RELEASE) {
            // Remove released keys from state map
            state = InputState::Released;
        }
    }
    // Remove released keys from state map
    for (auto it = keyboard.keyStates.begin(); it != keyboard.keyStates.end(); ) {
        if (it->second == InputState::Released) {
            it = keyboard.keyStates.erase(it);
        } else {
            ++it;
        }
    }
}

void InputManager::bindAction(const std::string& name, InputDevice device, int keyCode, 
                             InputAction action, std::function<void()> callback) {
    // Remove existing binding with same name
    unbindAction(name);
    
    // Add new binding
    bindings.emplace_back(name, device, keyCode, action, callback);
    VF_LOG_DEBUG("Bound action '{}' to key {}", name, keyCode);
}

void InputManager::unbindAction(const std::string& name) {
    bindings.erase(
        std::remove_if(bindings.begin(), bindings.end(),
                      [&name](const InputBinding& binding) { return binding.name == name; }),
        bindings.end()
    );
}

void InputManager::clearBindings() {
    bindings.clear();
}

bool InputManager::isKeyPressed(KeyCode key) const {
    auto it = keyboard.keyStates.find(static_cast<int>(key));
    return it != keyboard.keyStates.end() && it->second == InputState::Pressed;
}

bool InputManager::isKeyHeld(KeyCode key) const {
    auto it = keyboard.keyStates.find(static_cast<int>(key));
    return it != keyboard.keyStates.end() && it->second == InputState::Held;
}

bool InputManager::isKeyReleased(KeyCode key) const {
    auto it = keyboard.keyStates.find(static_cast<int>(key));
    return it != keyboard.keyStates.end() && it->second == InputState::Released;
}

bool InputManager::isMouseButtonPressed(int button) const {
    auto it = mouse.buttonStates.find(button);
    return it != mouse.buttonStates.end() && it->second == InputState::Pressed;
}

bool InputManager::isMouseButtonHeld(int button) const {
    auto it = mouse.buttonStates.find(button);
    return it != mouse.buttonStates.end() && it->second == InputState::Held;
}

bool InputManager::isMouseButtonReleased(int button) const {
    auto it = mouse.buttonStates.find(button);
    return it != mouse.buttonStates.end() && it->second == InputState::Released;
}

void InputManager::getMousePosition(double& x, double& y) const {
    x = mouse.x;
    y = mouse.y;
}

void InputManager::getMouseDelta(double& deltaX, double& deltaY) const {
    deltaX = mouse.deltaX;
    deltaY = mouse.deltaY;
}

void InputManager::getMouseScroll(double& scrollX, double& scrollY) const {
    scrollX = mouse.scrollX;
    scrollY = mouse.scrollY;
}

bool InputManager::isGamepadConnected(int gamepadId) const {
    auto it = gamepads.find(gamepadId);
    return it != gamepads.end() && it->second.connected;
}

float InputManager::getGamepadAxis(int gamepadId, int axis) const {
    auto it = gamepads.find(gamepadId);
    if (it != gamepads.end() && it->second.connected && axis < it->second.axes.size()) {
        return it->second.axes[axis];
    }
    return 0.0f;
}

bool InputManager::isGamepadButtonPressed(int gamepadId, int button) const {
    auto it = gamepads.find(gamepadId);
    if (it != gamepads.end() && it->second.connected && button < it->second.buttons.size()) {
        return it->second.buttons[button] == GLFW_PRESS;
    }
    return false;
}

void InputManager::setCursorVisible(bool visible) {
    if (window) {
        glfwSetInputMode(window, GLFW_CURSOR, 
                        visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    }
}

void InputManager::setCursorMode(int mode) {
    if (window) {
        glfwSetInputMode(window, GLFW_CURSOR, mode);
    }
}

void InputManager::resetState() {
    keyboard.keyStates.clear();
    keyboard.pressedKeys.clear();
    keyboard.releasedKeys.clear();
    mouse.buttonStates.clear();
    mouse.deltaX = 0.0;
    mouse.deltaY = 0.0;
    mouse.scrollX = 0.0;
    mouse.scrollY = 0.0;
}

// GLFW Callback Functions
void InputManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    InputManager& inputManager = getInstance();
    inputManager.updateKeyState(key, action);
}

void InputManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    InputManager& inputManager = getInstance();
    inputManager.updateMouseButtonState(button, action);
}

void InputManager::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    InputManager& inputManager = getInstance();
    inputManager.mouse.x = xpos;
    inputManager.mouse.y = ypos;
}

void InputManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    InputManager& inputManager = getInstance();
    inputManager.mouse.scrollX = xoffset;
    inputManager.mouse.scrollY = yoffset;
}

void InputManager::gamepadCallback(int gamepadId, int event) {
    InputManager& inputManager = getInstance();
    if (event == GLFW_CONNECTED) {
        inputManager.updateGamepadState(gamepadId);
        VF_LOG_INFO("Gamepad {} connected", gamepadId);
    } else if (event == GLFW_DISCONNECTED) {
        inputManager.gamepads[gamepadId].connected = false;
        VF_LOG_INFO("Gamepad {} disconnected", gamepadId);
    }
}

// Helper Functions
void InputManager::processBindings() {
    for (const auto& binding : bindings) {
        bool shouldTrigger = false;
        
        switch (binding.device) {
            case InputDevice::Keyboard: {
                auto it = keyboard.keyStates.find(binding.keyCode);
                if (it != keyboard.keyStates.end()) {
                    switch (binding.action) {
                        case InputAction::Press:
                            shouldTrigger = (it->second == InputState::Pressed);
                            break;
                        case InputAction::Release:
                            shouldTrigger = (it->second == InputState::Released);
                            break;
                        case InputAction::Hold:
                            shouldTrigger = (it->second == InputState::Held);
                            break;
                        case InputAction::Repeat:
                            shouldTrigger = (it->second == InputState::Held);
                            break;
                    }
                }
                break;
            }
            case InputDevice::Mouse: {
                auto it = mouse.buttonStates.find(binding.keyCode);
                if (it != mouse.buttonStates.end()) {
                    switch (binding.action) {
                        case InputAction::Press:
                            shouldTrigger = (it->second == InputState::Pressed);
                            break;
                        case InputAction::Release:
                            shouldTrigger = (it->second == InputState::Released);
                            break;
                        case InputAction::Hold:
                            shouldTrigger = (it->second == InputState::Held);
                            break;
                        case InputAction::Repeat:
                            shouldTrigger = (it->second == InputState::Held);
                            break;
                    }
                }
                break;
            }
            case InputDevice::Gamepad:
                // Gamepad binding implementation would go here
                break;
        }
        
        if (shouldTrigger && binding.callback) {
            binding.callback();
        }
    }
}

void InputManager::updateKeyState(int key, int action) {
    switch (action) {
        case GLFW_PRESS:
            keyboard.keyStates[key] = InputState::Pressed;
            keyboard.pressedKeys.push_back(key);
            break;
        case GLFW_RELEASE:
            keyboard.keyStates[key] = InputState::Released;
            keyboard.releasedKeys.push_back(key);
            break;
        case GLFW_REPEAT:
            keyboard.keyStates[key] = InputState::Held;
            break;
    }
}

void InputManager::updateMouseButtonState(int button, int action) {
    switch (action) {
        case GLFW_PRESS:
            mouse.buttonStates[button] = InputState::Pressed;
            break;
        case GLFW_RELEASE:
            mouse.buttonStates[button] = InputState::Released;
            break;
    }
}

void InputManager::updateGamepadState(int gamepadId) {
    if (!glfwJoystickPresent(gamepadId)) {
        gamepads[gamepadId].connected = false;
        return;
    }
    
    GamepadState& state = gamepads[gamepadId];
    state.connected = true;
    state.name = glfwGetJoystickName(gamepadId);
    
    // Get axes
    int axesCount;
    const float* axes = glfwGetJoystickAxes(gamepadId, &axesCount);
    state.axes.clear();
    state.axes.reserve(axesCount);
    for (int i = 0; i < axesCount; ++i) {
        state.axes.push_back(axes[i]);
    }
    
    // Get buttons
    int buttonCount;
    const unsigned char* buttons = glfwGetJoystickButtons(gamepadId, &buttonCount);
    state.buttons.clear();
    state.buttons.reserve(buttonCount);
    for (int i = 0; i < buttonCount; ++i) {
        state.buttons.push_back(buttons[i]);
    }
}

} // namespace Core
} // namespace VaporFrame 