#include "UISystem.h"
#include "ImGuiUI.h"
#include "WebViewUI.h"
#include "Logger.h"
#include "InputManager.h"
#include "SceneGraph.h"
#include "MemoryManager.h"
#include "Camera.h"

namespace VaporFrame::Core {

UISystem::UISystem()
    : initialized(false)
    , uiVisible(true)
    , debugUIVisible(true)
    , inputConsumed(false)
    , currentTheme("default")
    , frameTime(0.0f)
    , uiRenderTime(0.0f)
{
    VF_LOG_INFO("UISystem created");
}

UISystem::~UISystem() {
    shutdown();
    VF_LOG_INFO("UISystem destroyed");
}

bool UISystem::initialize() {
    if (initialized) {
        VF_LOG_WARN("UISystem already initialized");
        return true;
    }

    VF_LOG_INFO("Initializing UISystem");

    // Initialize ImGui UI
    imGuiUI = std::make_unique<ImGuiUI>();
    
    // Note: We'll need to get these from VulkanRenderer later
    // For now, we'll initialize ImGui without Vulkan resources
    // and set them up when the renderer is ready
    
    initialized = true;
    VF_LOG_INFO("UISystem initialized successfully");
    return true;
}

void UISystem::shutdown() {
    if (!initialized) return;

    VF_LOG_INFO("Shutting down UISystem");

    // Shutdown WebView UIs
    for (auto& webView : webViewUIs) {
        webView->shutdown();
    }
    webViewUIs.clear();

    // Shutdown ImGui UI
    if (imGuiUI) {
        imGuiUI->shutdown();
        imGuiUI.reset();
    }

    initialized = false;
    VF_LOG_INFO("UISystem shutdown complete");
}

void UISystem::update(float deltaTime) {
    if (!initialized || !uiVisible) return;

    frameTime = deltaTime;
    
    // Update ImGui UI
    if (imGuiUI) {
        imGuiUI->update(deltaTime);
    }
    
    // Update WebView UIs
    for (auto& webView : webViewUIs) {
        if (webView->isVisible()) {
            webView->update(deltaTime);
        }
    }
    
    updatePerformanceMetrics(deltaTime);
}

void UISystem::render(VkCommandBuffer commandBuffer) {
    if (!initialized || !uiVisible) return;

    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Render WebView UIs first (background)
    for (auto& webView : webViewUIs) {
        if (webView->isVisible()) {
            webView->render(commandBuffer);
        }
    }
    
    // Render ImGui UI on top
    if (imGuiUI) {
        imGuiUI->render(commandBuffer);
    }
    
    // Render debug UI if enabled
    if (debugUIVisible) {
        renderDebugUI();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    uiRenderTime = std::chrono::duration<float>(endTime - startTime).count();
}

void UISystem::renderSimple() {
    if (!initialized || !uiVisible) return;

    VF_LOG_INFO("UISystem::renderSimple() called");
    
    // Simple rendering without Vulkan command buffer
    // This is a temporary solution until we integrate proper UI rendering
    
    // Render WebView UIs
    for (auto& webView : webViewUIs) {
        if (webView->isVisible()) {
            VF_LOG_INFO("Rendering WebView UI: visible at ({}, {}) with size {}x{}", 
                       webView->getX(), webView->getY(), webView->getWidth(), webView->getHeight());
        }
    }
    
    // Render ImGui UI
    if (imGuiUI) {
        VF_LOG_INFO("Rendering ImGui UI");
    }
    
    // Render debug UI if enabled
    if (debugUIVisible) {
        renderDebugUI();
    }
}

void UISystem::handleInput(const UIEvent& event) {
    if (!initialized || !uiVisible) return;

    inputConsumed = false;
    
    // Handle input for WebView UIs first (they're in the background)
    for (auto& webView : webViewUIs) {
        if (webView->isVisible() && webView->isPointInside(event.x, event.y)) {
            switch (event.type) {
                case UIEventType::MouseMove:
                    webView->handleMouseMove(event.x, event.y);
                    break;
                case UIEventType::MouseClick:
                    webView->handleMouseClick(event.button, event.pressed);
                    break;
                case UIEventType::MouseScroll:
                    webView->handleMouseScroll(event.x, event.y);
                    break;
                case UIEventType::KeyPress:
                case UIEventType::KeyRelease:
                    webView->handleKeyPress(event.key, event.pressed);
                    break;
                case UIEventType::TextInput:
                    webView->handleTextInput(event.text);
                    break;
            }
            inputConsumed = true;
            break;
        }
    }
    
    // If WebView didn't consume input, pass to ImGui
    if (!inputConsumed && imGuiUI) {
        switch (event.type) {
            case UIEventType::MouseMove:
                imGuiUI->handleMouseMove(event.x, event.y);
                break;
            case UIEventType::MouseClick:
                imGuiUI->handleMouseClick(event.button, event.pressed);
                break;
            case UIEventType::MouseScroll:
                imGuiUI->handleMouseScroll(event.x, event.y);
                break;
            case UIEventType::KeyPress:
            case UIEventType::KeyRelease:
                imGuiUI->handleKeyPress(event.key, event.pressed);
                break;
            case UIEventType::TextInput:
                imGuiUI->handleTextInput(event.text);
                break;
        }
        inputConsumed = true;
    }
}

ImGuiUI* UISystem::createImGuiUI(const std::string& name) {
    if (!initialized) {
        VF_LOG_ERROR("Cannot create ImGui UI - UISystem not initialized");
        return nullptr;
    }
    
    if (imGuiUI) {
        VF_LOG_WARN("ImGui UI already exists, returning existing instance");
        return imGuiUI.get();
    }
    
    imGuiUI = std::make_unique<ImGuiUI>();
    VF_LOG_INFO("Created ImGui UI: {}", name);
    return imGuiUI.get();
}

WebViewUI* UISystem::createWebViewUI(const std::string& name, const std::string& htmlPath) {
    if (!initialized) {
        VF_LOG_ERROR("Cannot create WebView UI - UISystem not initialized");
        return nullptr;
    }
    
    auto webView = std::make_unique<WebViewUI>();
    if (!webView->initialize(htmlPath)) {
        VF_LOG_ERROR("Failed to initialize WebView UI: {}", name);
        return nullptr;
    }
    
    WebViewUI* result = webView.get();
    webViewUIs.push_back(std::move(webView));
    
    VF_LOG_INFO("Created WebView UI: {} with HTML: {}", name, htmlPath);
    return result;
}

void UISystem::setGlobalTheme(const std::string& theme) {
    currentTheme = theme;
    
    // Apply theme to all WebView UIs
    for (auto& webView : webViewUIs) {
        webView->setTheme(theme);
    }
    
    VF_LOG_INFO("Set global UI theme: {}", theme);
}

void UISystem::reloadAllAssets() {
    VF_LOG_INFO("Reloading all UI assets");
    
    // Reload WebView assets
    for (auto& webView : webViewUIs) {
        webView->reloadAssets();
    }
}

void UISystem::hotReloadAssets() {
    // This will be implemented with file watching
    VF_LOG_INFO("Hot reloading UI assets");
    reloadAllAssets();
}

void UISystem::initializeImGui() {
    // This will be called when Vulkan resources are available
    VF_LOG_INFO("Initializing ImGui with Vulkan resources");
}

void UISystem::shutdownImGui() {
    if (imGuiUI) {
        imGuiUI->shutdown();
    }
}

void UISystem::renderDebugUI() {
    // Simple debug overlay showing UI system status
    // This will be enhanced with more detailed information
}

void UISystem::updatePerformanceMetrics(float deltaTime) {
    // Update performance tracking data
    // This will be used for the performance panel
}

} // namespace VaporFrame::Core 