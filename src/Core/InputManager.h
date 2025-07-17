#pragma once

#include <GLFW/glfw3.h>
#include <unordered_map>
#include <vector>
#include <functional>
#include <string>

namespace VaporFrame {
namespace Core {

// Input action types
enum class InputAction {
    Press,
    Release,
    Repeat,
    Hold
};

// Input device types
enum class InputDevice {
    Keyboard,
    Mouse,
    Gamepad
};

// Input state
enum class InputState {
    Released,
    Pressed,
    Held
};

// Key codes (mapped to GLFW keys)
enum class KeyCode {
    // Letters
    A = GLFW_KEY_A, B = GLFW_KEY_B, C = GLFW_KEY_C, D = GLFW_KEY_D,
    E = GLFW_KEY_E, F = GLFW_KEY_F, G = GLFW_KEY_G, H = GLFW_KEY_H,
    I = GLFW_KEY_I, J = GLFW_KEY_J, K = GLFW_KEY_K, L = GLFW_KEY_L,
    M = GLFW_KEY_M, N = GLFW_KEY_N, O = GLFW_KEY_O, P = GLFW_KEY_P,
    Q = GLFW_KEY_Q, R = GLFW_KEY_R, S = GLFW_KEY_S, T = GLFW_KEY_T,
    U = GLFW_KEY_U, V = GLFW_KEY_V, W = GLFW_KEY_W, X = GLFW_KEY_X,
    Y = GLFW_KEY_Y, Z = GLFW_KEY_Z,
    
    // Numbers
    Zero = GLFW_KEY_0, One = GLFW_KEY_1, Two = GLFW_KEY_2, Three = GLFW_KEY_3,
    Four = GLFW_KEY_4, Five = GLFW_KEY_5, Six = GLFW_KEY_6, Seven = GLFW_KEY_7,
    Eight = GLFW_KEY_8, Nine = GLFW_KEY_9,
    
    // Special keys
    Space = GLFW_KEY_SPACE, Enter = GLFW_KEY_ENTER, Tab = GLFW_KEY_TAB,
    Escape = GLFW_KEY_ESCAPE, Backspace = GLFW_KEY_BACKSPACE, Delete = GLFW_KEY_DELETE,
    
    // Arrow keys
    Up = GLFW_KEY_UP, Down = GLFW_KEY_DOWN, Left = GLFW_KEY_LEFT, Right = GLFW_KEY_RIGHT,
    
    // Function keys
    F1 = GLFW_KEY_F1, F2 = GLFW_KEY_F2, F3 = GLFW_KEY_F3, F4 = GLFW_KEY_F4,
    F5 = GLFW_KEY_F5, F6 = GLFW_KEY_F6, F7 = GLFW_KEY_F7, F8 = GLFW_KEY_F8,
    F9 = GLFW_KEY_F9, F10 = GLFW_KEY_F10, F11 = GLFW_KEY_F11, F12 = GLFW_KEY_F12,
    
    // Modifiers
    Shift = GLFW_KEY_LEFT_SHIFT, Ctrl = GLFW_KEY_LEFT_CONTROL, Alt = GLFW_KEY_LEFT_ALT,
    
    // Mouse buttons
    MouseLeft = GLFW_MOUSE_BUTTON_LEFT,
    MouseRight = GLFW_MOUSE_BUTTON_RIGHT,
    MouseMiddle = GLFW_MOUSE_BUTTON_MIDDLE
};

// Input binding structure
struct InputBinding {
    std::string name;
    InputDevice device;
    int keyCode;
    InputAction action;
    std::function<void()> callback;
    
    InputBinding(const std::string& n, InputDevice dev, int key, InputAction act, std::function<void()> cb)
        : name(n), device(dev), keyCode(key), action(act), callback(cb) {}
};

// Mouse state
struct MouseState {
    double x = 0.0;
    double y = 0.0;
    double deltaX = 0.0;
    double deltaY = 0.0;
    double scrollX = 0.0;
    double scrollY = 0.0;
    std::unordered_map<int, InputState> buttonStates;
};

// Keyboard state
struct KeyboardState {
    std::unordered_map<int, InputState> keyStates;
    std::vector<int> pressedKeys;
    std::vector<int> releasedKeys;
};

// Gamepad state
struct GamepadState {
    bool connected = false;
    std::string name;
    std::vector<float> axes;
    std::vector<unsigned char> buttons;
};

class InputManager {
public:
    static InputManager& getInstance();
    
    // Initialization and cleanup
    void initialize(GLFWwindow* window);
    void shutdown();
    
    // Update input state (call once per frame)
    void update();
    
    // Input binding
    void bindAction(const std::string& name, InputDevice device, int keyCode, 
                   InputAction action, std::function<void()> callback);
    void unbindAction(const std::string& name);
    void clearBindings();
    
    // Input querying
    bool isKeyPressed(KeyCode key) const;
    bool isKeyHeld(KeyCode key) const;
    bool isKeyReleased(KeyCode key) const;
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonHeld(int button) const;
    bool isMouseButtonReleased(int button) const;
    
    // Mouse position and movement
    void getMousePosition(double& x, double& y) const;
    void getMouseDelta(double& deltaX, double& deltaY) const;
    void getMouseScroll(double& scrollX, double& scrollY) const;
    
    // Gamepad support
    bool isGamepadConnected(int gamepadId = GLFW_JOYSTICK_1) const;
    float getGamepadAxis(int gamepadId, int axis) const;
    bool isGamepadButtonPressed(int gamepadId, int button) const;
    
    // Cursor control
    void setCursorVisible(bool visible);
    void setCursorMode(int mode); // GLFW_CURSOR_NORMAL, GLFW_CURSOR_HIDDEN, GLFW_CURSOR_DISABLED
    
    // Input state reset (useful for scene transitions)
    void resetState();
    
private:
    InputManager() = default;
    ~InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
    
    // GLFW callback functions
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void gamepadCallback(int gamepadId, int event);
    
    // Internal state
    GLFWwindow* window = nullptr;
    KeyboardState keyboard;
    MouseState mouse;
    std::unordered_map<int, GamepadState> gamepads;
    std::vector<InputBinding> bindings;
    
    // Previous frame state for delta calculations
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    
    // Helper functions
    void processBindings();
    void updateKeyState(int key, int action);
    void updateMouseButtonState(int button, int action);
    void updateGamepadState(int gamepadId);
};

// Convenience functions for common input queries
inline bool IsKeyPressed(KeyCode key) {
    return InputManager::getInstance().isKeyPressed(key);
}

inline bool IsKeyHeld(KeyCode key) {
    return InputManager::getInstance().isKeyHeld(key);
}

inline bool IsKeyReleased(KeyCode key) {
    return InputManager::getInstance().isKeyReleased(key);
}

inline bool IsMouseButtonPressed(int button) {
    return InputManager::getInstance().isMouseButtonPressed(button);
}

inline bool IsMouseButtonHeld(int button) {
    return InputManager::getInstance().isMouseButtonHeld(button);
}

inline bool IsMouseButtonReleased(int button) {
    return InputManager::getInstance().isMouseButtonReleased(button);
}

} // namespace Core
} // namespace VaporFrame 