#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "MemoryManager.h"

// Forward declarations
struct VkCommandBuffer_T;
typedef VkCommandBuffer_T* VkCommandBuffer;
struct VkDevice_T;
typedef VkDevice_T* VkDevice;
struct VkRenderPass_T;
typedef VkRenderPass_T* VkRenderPass;
struct VkPipelineLayout_T;
typedef VkPipelineLayout_T* VkPipelineLayout;
struct VkDescriptorPool_T;
typedef VkDescriptorPool_T* VkDescriptorPool;

namespace VaporFrame::Core {

// Forward declarations
class SceneManager;
class InputManager;
class MemoryManager;
class Camera;

class ImGuiUI {
public:
    ImGuiUI();
    ~ImGuiUI();

    // System management
    bool initialize(VkDevice device, VkRenderPass renderPass, VkPipelineLayout pipelineLayout);
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
    
    // Panel visibility
    void setPerformancePanelVisible(bool visible) { performancePanelVisible = visible; }
    void setMemoryPanelVisible(bool visible) { memoryPanelVisible = visible; }
    void setSceneHierarchyVisible(bool visible) { sceneHierarchyVisible = visible; }
    void setAssetBrowserVisible(bool visible) { assetBrowserVisible = visible; }
    void setConsoleVisible(bool visible) { consoleVisible = visible; }
    void setInspectorVisible(bool visible) { inspectorVisible = visible; }
    
    // Engine integration
    void setSceneManager(SceneManager* sceneManager) { this->sceneManager = sceneManager; }
    void setInputManager(InputManager* inputManager) { this->inputManager = inputManager; }
    void setMemoryManager(MemoryManager* memoryManager) { this->memoryManager = memoryManager; }
    void setCamera(Camera* camera) { this->camera = camera; }
    
    // Console functionality
    void addConsoleMessage(const std::string& message, const std::string& level = "info");
    void clearConsole();
    
    // Inspector functionality
    void setSelectedEntity(uint32_t entityId) { selectedEntityId = entityId; }
    uint32_t getSelectedEntity() const { return selectedEntityId; }

private:
    // Vulkan resources
    VkDevice device;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkDescriptorPool descriptorPool;
    
    // Engine references
    SceneManager* sceneManager;
    InputManager* inputManager;
    MemoryManager* memoryManager;
    Camera* camera;
    
    // Panel visibility states
    bool performancePanelVisible;
    bool memoryPanelVisible;
    bool sceneHierarchyVisible;
    bool assetBrowserVisible;
    bool consoleVisible;
    bool inspectorVisible;
    
    // Console state
    std::vector<std::pair<std::string, std::string>> consoleMessages; // message, level
    char consoleInputBuffer[256];
    bool consoleAutoScroll;
    
    // Inspector state
    uint32_t selectedEntityId;
    
    // Performance tracking
    struct PerformanceData {
        float fps;
        float frameTime;
        float cpuTime;
        float gpuTime;
        uint32_t drawCalls;
        uint32_t triangles;
    } performanceData;
    
    // Memory tracking
    struct MemoryStats memoryData;
    
    // Private methods
    void renderPerformancePanel();
    void renderMemoryPanel();
    void renderSceneHierarchy();
    void renderAssetBrowser();
    void renderConsole();
    void renderInspector();
    void renderMainMenuBar();
    
    void updatePerformanceData(float deltaTime);
    void updateMemoryData();
    void updateSceneData();
    
    // Utility methods
    void renderEntityNode(uint32_t entityId, const std::string& name);
    void renderComponentInspector(uint32_t entityId);
    std::string formatBytes(size_t bytes);
    std::string formatTime(float seconds);
};

} // namespace VaporFrame::Core 