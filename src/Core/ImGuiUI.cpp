#include "ImGuiUI.h"
#include "Logger.h"
#include "SceneGraph.h"
#include "InputManager.h"
#include "MemoryManager.h"
#include "Camera.h"

// Dear ImGui includes
#include "imgui.h"
#if !defined(IMGUI_VERSION_NUM) || IMGUI_VERSION_NUM < 19000
#error "Dear ImGui version is too old or not found!"
#endif
#pragma message ("Dear ImGui version: " IMGUI_VERSION)
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

// GLFW include
#include <GLFW/glfw3.h>

// Standard includes
#include <chrono>
#include <sstream>
#include <iomanip>

namespace VaporFrame::Core {

ImGuiUI::ImGuiUI()
    : device(VK_NULL_HANDLE)
    , renderPass(VK_NULL_HANDLE)
    , pipelineLayout(VK_NULL_HANDLE)
    , descriptorPool(VK_NULL_HANDLE)
    , sceneManager(nullptr)
    , inputManager(nullptr)
    , memoryManager(nullptr)
    , camera(nullptr)
    , performancePanelVisible(true)
    , memoryPanelVisible(true)
    , sceneHierarchyVisible(true)
    , assetBrowserVisible(false)
    , consoleVisible(true)
    , inspectorVisible(true)
    , consoleAutoScroll(true)
    , selectedEntityId(0)
{
    memset(consoleInputBuffer, 0, sizeof(consoleInputBuffer));
    memset(&performanceData, 0, sizeof(performanceData));
    
    VF_LOG_INFO("ImGuiUI created");
}

ImGuiUI::~ImGuiUI() {
    shutdown();
    VF_LOG_INFO("ImGuiUI destroyed");
}

bool ImGuiUI::initialize(VkDevice device, VkRenderPass renderPass, VkPipelineLayout pipelineLayout) {
    if (this->device != VK_NULL_HANDLE) {
        VF_LOG_WARN("ImGuiUI already initialized");
        return true;
    }

    this->device = device;
    this->renderPass = renderPass;
    this->pipelineLayout = pipelineLayout;

    VF_LOG_INFO("Initializing ImGuiUI with Vulkan");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Note: Docking and viewports require specific ImGui configuration
    // For now, we'll use basic ImGui without these advanced features
    // These can be enabled later when we properly configure ImGui with docking support

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    // Basic styling for now
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;

    // Setup Platform/Renderer backends
    // Note: We'll need GLFW window handle from the main application
    // For now, we'll initialize without GLFW integration
    // ImGui_ImplGlfw_InitForVulkan(window, true);
    
    // ImGui_ImplVulkan_InitInfo init_info = {};
    // init_info.Instance = instance;
    // init_info.PhysicalDevice = physicalDevice;
    // init_info.Device = device;
    // init_info.QueueFamily = queueFamily;
    // init_info.Queue = queue;
    // init_info.PipelineCache = pipelineCache;
    // init_info.DescriptorPool = descriptorPool;
    // init_info.Subpass = 0;
    // init_info.MinImageCount = minImageCount;
    // init_info.ImageCount = imageCount;
    // init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    // init_info.Allocator = allocator;
    // init_info.CheckVkResultFn = check_vk_result;
    // ImGui_ImplVulkan_Init(&init_info);

    // Add some initial console messages
    addConsoleMessage("ImGui UI initialized successfully", "info");
    addConsoleMessage("Welcome to VaporFrame Engine", "info");

    VF_LOG_INFO("ImGuiUI initialized successfully");
    return true;
}

void ImGuiUI::shutdown() {
    if (device == VK_NULL_HANDLE) return;

    VF_LOG_INFO("Shutting down ImGuiUI");

    // ImGui_ImplVulkan_Shutdown();
    // ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    device = VK_NULL_HANDLE;
    renderPass = VK_NULL_HANDLE;
    pipelineLayout = VK_NULL_HANDLE;
    descriptorPool = VK_NULL_HANDLE;

    VF_LOG_INFO("ImGuiUI shutdown complete");
}

void ImGuiUI::update(float deltaTime) {
    if (device == VK_NULL_HANDLE) return;

    // Start the Dear ImGui frame
    // ImGui_ImplVulkan_NewFrame();
    // ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    updatePerformanceData(deltaTime);
    updateMemoryData();
    updateSceneData();

    // Render main menu bar
    renderMainMenuBar();

    // Render panels based on visibility
    if (performancePanelVisible) renderPerformancePanel();
    if (memoryPanelVisible) renderMemoryPanel();
    if (sceneHierarchyVisible) renderSceneHierarchy();
    if (assetBrowserVisible) renderAssetBrowser();
    if (consoleVisible) renderConsole();
    if (inspectorVisible) renderInspector();

    ImGui::Render();
}

void ImGuiUI::render(VkCommandBuffer commandBuffer) {
    if (device == VK_NULL_HANDLE) return;

    // Record ImGui draw commands
    // ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    // Update and Render additional Platform Windows
    // Note: Viewport support would be enabled here when using docking branch
    // ImGuiIO& io = ImGui::GetIO();
    // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    //     ImGui::UpdatePlatformWindows();
    //     ImGui::RenderPlatformWindowsDefault();
    // }
}

void ImGuiUI::handleMouseMove(double x, double y) {
    // ImGui_ImplGlfw_MousePosCallback(window, x, y);
}

void ImGuiUI::handleMouseClick(int button, bool pressed) {
    // ImGui_ImplGlfw_MouseButtonCallback(window, button, pressed ? GLFW_PRESS : GLFW_RELEASE, 0);
}

void ImGuiUI::handleMouseScroll(double xOffset, double yOffset) {
    // ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
}

void ImGuiUI::handleKeyPress(int key, bool pressed) {
    // ImGui_ImplGlfw_KeyCallback(window, key, 0, pressed ? GLFW_PRESS : GLFW_RELEASE, 0);
}

void ImGuiUI::handleTextInput(const std::string& text) {
    // ImGui_ImplGlfw_CharCallback(window, text[0]);
}

void ImGuiUI::addConsoleMessage(const std::string& message, const std::string& level) {
    consoleMessages.push_back({message, level});
    if (consoleMessages.size() > 1000) {
        consoleMessages.erase(consoleMessages.begin());
    }
}

void ImGuiUI::clearConsole() {
    consoleMessages.clear();
}

void ImGuiUI::renderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene")) {
                addConsoleMessage("New Scene requested", "info");
            }
            if (ImGui::MenuItem("Open Scene")) {
                addConsoleMessage("Open Scene requested", "info");
            }
            if (ImGui::MenuItem("Save Scene")) {
                addConsoleMessage("Save Scene requested", "info");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                addConsoleMessage("Exit requested", "info");
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Performance Panel", nullptr, &performancePanelVisible);
            ImGui::MenuItem("Memory Panel", nullptr, &memoryPanelVisible);
            ImGui::MenuItem("Scene Hierarchy", nullptr, &sceneHierarchyVisible);
            ImGui::MenuItem("Asset Browser", nullptr, &assetBrowserVisible);
            ImGui::MenuItem("Console", nullptr, &consoleVisible);
            ImGui::MenuItem("Inspector", nullptr, &inspectorVisible);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                addConsoleMessage("VaporFrame Engine v0.1.0", "info");
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void ImGuiUI::renderPerformancePanel() {
    if (ImGui::Begin("Performance", &performancePanelVisible)) {
        ImGui::Text("FPS: %.1f", performanceData.fps);
        ImGui::Text("Frame Time: %.2f ms", performanceData.frameTime * 1000.0f);
        ImGui::Text("CPU Time: %.2f ms", performanceData.cpuTime * 1000.0f);
        ImGui::Text("GPU Time: %.2f ms", performanceData.gpuTime * 1000.0f);
        ImGui::Text("Draw Calls: %u", performanceData.drawCalls);
        ImGui::Text("Triangles: %u", performanceData.triangles);
        
        // Performance graph
        static float values[100] = {};
        static int values_offset = 0;
        values[values_offset] = performanceData.fps;
        values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
        
        ImGui::PlotLines("FPS Graph", values, IM_ARRAYSIZE(values), values_offset, nullptr, 0.0f, 200.0f, ImVec2(0, 80.0f));
    }
    ImGui::End();
}

void ImGuiUI::renderMemoryPanel() {
    if (ImGui::Begin("Memory", &memoryPanelVisible)) {
        ImGui::Text("Total Allocated: %s", formatBytes(memoryData.totalAllocated).c_str());
        ImGui::Text("Total Freed: %s", formatBytes(memoryData.totalFreed).c_str());
        ImGui::Text("Current Usage: %s", formatBytes(memoryData.currentUsage).c_str());
        ImGui::Text("Peak Usage: %s", formatBytes(memoryData.peakUsage).c_str());
        ImGui::Text("Allocation Count: %zu", memoryData.allocationCount);
        ImGui::Text("Deallocation Count: %zu", memoryData.deallocationCount);
        ImGui::Text("Fragmentation: %zu", memoryData.fragmentation);
        
        // Memory usage graph
        static float values[100] = {};
        static int values_offset = 0;
        values[values_offset] = static_cast<float>(memoryData.currentUsage) / (1024 * 1024); // Convert to MB
        values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
        
        ImGui::PlotLines("Memory Usage (MB)", values, IM_ARRAYSIZE(values), values_offset, nullptr, 0.0f, 1000.0f, ImVec2(0, 80.0f));
    }
    ImGui::End();
}

void ImGuiUI::renderSceneHierarchy() {
    if (ImGui::Begin("Scene Hierarchy", &sceneHierarchyVisible)) {
        if (sceneManager) {
            auto scene = sceneManager->getActiveScene();
            if (scene) {
                renderEntityNode(0, "Root");
                ImGui::TreePop();
            }
        } else {
            ImGui::Text("Scene Manager not available");
        }
    }
    ImGui::End();
}

void ImGuiUI::renderAssetBrowser() {
    if (ImGui::Begin("Asset Browser", &assetBrowserVisible)) {
        ImGui::Text("Asset Browser - Coming Soon");
        ImGui::Text("This will show available assets and allow drag & drop");
    }
    ImGui::End();
}

void ImGuiUI::renderConsole() {
    if (ImGui::Begin("Console", &consoleVisible)) {
        // Console output
        ImGui::BeginChild("ConsoleOutput", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        for (const auto& message : consoleMessages) {
            ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            if (message.second == "error") color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            else if (message.second == "warning") color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            else if (message.second == "info") color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
            
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextWrapped("[%s] %s", message.second.c_str(), message.first.c_str());
            ImGui::PopStyleColor();
        }
        
        if (consoleAutoScroll) {
            ImGui::SetScrollHereY(1.0f);
        }
        
        ImGui::EndChild();
        
        // Console input
        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##ConsoleInput", consoleInputBuffer, sizeof(consoleInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (strlen(consoleInputBuffer) > 0) {
                addConsoleMessage(std::string("> ") + consoleInputBuffer, "input");
                // TODO: Execute console command
                memset(consoleInputBuffer, 0, sizeof(consoleInputBuffer));
            }
        }
        ImGui::PopItemWidth();
        
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            clearConsole();
        }
    }
    ImGui::End();
}

void ImGuiUI::renderInspector() {
    if (ImGui::Begin("Inspector", &inspectorVisible)) {
        if (selectedEntityId != 0) {
            ImGui::Text("Entity ID: %u", selectedEntityId);
            renderComponentInspector(selectedEntityId);
        } else {
            ImGui::Text("No entity selected");
            ImGui::Text("Select an entity in the Scene Hierarchy");
        }
    }
    ImGui::End();
}

void ImGuiUI::renderEntityNode(uint32_t entityId, const std::string& name) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (selectedEntityId == entityId) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    
    bool isOpen = ImGui::TreeNodeEx(name.c_str(), flags);
    if (ImGui::IsItemClicked()) {
        selectedEntityId = entityId;
    }
    
    if (isOpen) {
        // TODO: Render child entities
        ImGui::TreePop();
    }
}

void ImGuiUI::renderComponentInspector(uint32_t entityId) {
    ImGui::Text("Components:");
    ImGui::Separator();
    
    // TODO: Render actual components from the entity
    ImGui::Text("Transform Component");
    ImGui::Text("Mesh Component");
    ImGui::Text("Camera Component");
}

void ImGuiUI::updatePerformanceData(float deltaTime) {
    performanceData.frameTime = deltaTime;
    performanceData.fps = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;
    
    // TODO: Get actual performance data from renderer
    performanceData.cpuTime = deltaTime * 0.8f; // Placeholder
    performanceData.gpuTime = deltaTime * 0.2f; // Placeholder
    performanceData.drawCalls = 100; // Placeholder
    performanceData.triangles = 1000; // Placeholder
}

void ImGuiUI::updateMemoryData() {
    auto& memoryManager = MemoryManager::getInstance();
    auto stats = memoryManager.getGlobalStats();
    memoryData = stats;
}

void ImGuiUI::updateSceneData() {
    // Scene data is updated when rendering the scene hierarchy
}

std::string ImGuiUI::formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

std::string ImGuiUI::formatTime(float seconds) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << (seconds * 1000.0f) << " ms";
    return oss.str();
}

} // namespace VaporFrame::Core 