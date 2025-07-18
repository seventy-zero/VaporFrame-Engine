#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vulkan/vulkan.h>

namespace VaporFrame::Core {

class WebViewUI {
public:
    WebViewUI();
    ~WebViewUI();

    // System management
    bool initialize(const std::string& htmlPath, const std::string& cssPath = "");
    void shutdown();
    
    // Update and render
    void update(float deltaTime);
    void render(VkCommandBuffer commandBuffer);
    
    // Input handling
    void handleMouseMove(double x, double y);
    void handleMouseClick(int button, bool pressed);
    void handleMouseScroll(double xOffset, double yOffset);
    void handleKeyPress(int key, bool pressed);
    void handleTextInput(const std::string& text);
    
    // JavaScript bridge
    void registerCallback(const std::string& name, std::function<void(const std::string&)> callback);
    void executeJavaScript(const std::string& script);
    void callJavaScriptFunction(const std::string& functionName, const std::string& parameters = "");
    
    // Asset management
    void reloadAssets();
    void setTheme(const std::string& themeName);
    void setHTMLContent(const std::string& html);
    void setCSSContent(const std::string& css);
    
    // UI state
    void setVisible(bool visible) { this->visible = visible; }
    bool isVisible() const { return visible; }
    void setPosition(float x, float y) { this->x = x; this->y = y; }
    void setSize(float width, float height) { this->width = width; this->height = height; }
    
    // Getters for position and size
    float getX() const { return x; }
    float getY() const { return y; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }
    
    // Hit testing
    bool isPointInside(double x, double y) const;
    
    // Event handling
    void setOnLoadCallback(std::function<void()> callback) { onLoadCallback = callback; }
    void setOnErrorCallback(std::function<void(const std::string&)> callback) { onErrorCallback = callback; }

private:
    // WebView state
    bool initialized;
    bool visible;
    float x, y, width, height;
    
    // Asset paths
    std::string htmlPath;
    std::string cssPath;
    std::string currentTheme;
    
    // Content
    std::string htmlContent;
    std::string cssContent;
    
    // JavaScript bridge
    std::unordered_map<std::string, std::function<void(const std::string&)>> callbacks;
    
    // Event callbacks
    std::function<void()> onLoadCallback;
    std::function<void(const std::string&)> onErrorCallback;
    
    // Performance tracking
    float loadTime;
    float renderTime;
    
    // Private methods
    bool loadHTMLFile(const std::string& path);
    bool loadCSSFile(const std::string& path);
    void injectCSS(const std::string& css);
    void injectJavaScript(const std::string& script);
    
    // Simple HTML renderer implementation
    void* webViewHandle; // Placeholder for now
    
    // Vulkan texture for rendering
    VkImage webViewTexture;
    VkDeviceMemory webViewTextureMemory;
    VkImageView webViewTextureView;
    VkSampler webViewTextureSampler;
    bool textureCreated;
    
    // Utility methods
    std::string resolveAssetPath(const std::string& path);
    void logError(const std::string& error);
    void logInfo(const std::string& info);
    
    // UI rendering properties
    float uiColor[4]; // RGBA color for UI background
};

} // namespace VaporFrame::Core 