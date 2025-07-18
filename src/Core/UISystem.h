#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "WebViewUI.h"

// Forward declarations
struct VkCommandBuffer_T;
typedef VkCommandBuffer_T* VkCommandBuffer;

namespace VaporFrame::Core {

// Forward declarations
class ImGuiUI;
class WebViewUI;

enum class UIEventType {
    MouseMove,
    MouseClick,
    MouseScroll,
    KeyPress,
    KeyRelease,
    TextInput
};

struct UIEvent {
    UIEventType type;
    double x, y;           // For mouse events
    int button;            // For mouse clicks
    int key;               // For key events
    bool pressed;          // For button/key states
    std::string text;      // For text input
};

class UISystem {
public:
    UISystem();
    ~UISystem();

    // System management
    bool initialize();
    void shutdown();
    
    // Update and render
    void update(float deltaTime);
    void render(VkCommandBuffer commandBuffer);
    void renderSimple(); // Simple rendering without Vulkan command buffer
    
    // Input handling
    void handleInput(const UIEvent& event);
    bool isInputConsumed() const { return inputConsumed; }
    
    // UI creation and management
    ImGuiUI* createImGuiUI(const std::string& name);
    WebViewUI* createWebViewUI(const std::string& name, const std::string& htmlPath);
    
    // Global UI settings
    void setGlobalTheme(const std::string& theme);
    void setUIVisible(bool visible) { uiVisible = visible; }
    bool isUIVisible() const { return uiVisible; }
    
    // Debug and development
    void toggleDebugUI() { debugUIVisible = !debugUIVisible; }
    bool isDebugUIVisible() const { return debugUIVisible; }
    
    // Asset management
    void reloadAllAssets();
    void hotReloadAssets();

private:
    // UI components
    std::unique_ptr<ImGuiUI> imGuiUI;
    std::vector<std::unique_ptr<WebViewUI>> webViewUIs;
    
    // System state
    bool initialized;
    bool uiVisible;
    bool debugUIVisible;
    bool inputConsumed;
    
    // Theme and styling
    std::string currentTheme;
    
    // Performance tracking
    float frameTime;
    float uiRenderTime;
    
    // Private methods
    void initializeImGui();
    void shutdownImGui();
    void renderDebugUI();
    void updatePerformanceMetrics(float deltaTime);
};

} // namespace VaporFrame::Core 