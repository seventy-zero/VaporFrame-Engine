# Only build the main engine if Vulkan is available
if(VULKAN_AVAILABLE)
    # We'll change VaporFrameCore to be an executable for now to test main.cpp
    add_executable(VaporFrameEngine
        main.cpp
        VulkanRenderer.cpp
        Core/Logger.cpp
        Core/MemoryManager.cpp
        Core/InputManager.cpp
        Core/Camera.cpp
        Core/SceneGraph.cpp
        Core/MeshLoader.cpp
        Core/UISystem.cpp
        Core/ImGuiUI.cpp
        Core/WebViewUI.cpp
        # ImGui core and backends
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
        # Headers are usually not listed here but found via include paths
    )
else()
    message(STATUS "Skipping VaporFrameEngine build - Vulkan not available")
endif()

# Memory management test executable
add_executable(MemoryTest
    Core/MemoryTest.cpp
    Core/MemoryManager.cpp
    Core/Logger.cpp
)

# Input management test executable
add_executable(InputTest
    ../tests/InputTest.cpp
    Core/InputManager.cpp
    Core/Logger.cpp
)

# Camera test executable
add_executable(CameraTest
    ../tests/CameraTest.cpp
    Core/Camera.cpp
    Core/InputManager.cpp
    Core/Logger.cpp
)

# Scene Graph test executable
add_executable(SceneGraphTest
    ../tests/SceneGraphTest.cpp
    Core/SceneGraph.cpp
    Core/Logger.cpp
    Core/MeshLoader.cpp
)

# target_sources(VaporFrameCore
# PRIVATE
# main.cpp
#         # Add core engine source files here later
#         # Example: MainLoop.cpp, Window.cpp, etc.
# )

if(VULKAN_AVAILABLE)
    target_include_directories(VaporFrameEngine # Changed target name
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR} # For includes within src
            ${PROJECT_SOURCE_DIR}/third_party/glfw-3.4/include # To find <GLFW/glfw3.h>
            ${imgui_SOURCE_DIR} # To find <imgui.h>
        PRIVATE
            # Any private include directories for the core
    )

    target_link_libraries(VaporFrameEngine
        PUBLIC
            glfw
            Vulkan::Vulkan
            glm::glm
            spdlog::spdlog
            mimalloc
        PRIVATE
            # Any private link dependencies
    )
endif()

target_link_libraries(MemoryTest
    PUBLIC
        spdlog::spdlog
        mimalloc
    PRIVATE
        # Any private link dependencies
)

target_link_libraries(InputTest
    PUBLIC
        glfw
        spdlog::spdlog
        mimalloc
    PRIVATE
        # Any private link dependencies
)

target_link_libraries(CameraTest
    PUBLIC
        glfw
        glm::glm
        spdlog::spdlog
        mimalloc
    PRIVATE
        # Any private link dependencies
)

target_link_libraries(SceneGraphTest
    PUBLIC
        glm::glm
        spdlog::spdlog
        mimalloc
    PRIVATE
        # Any private link dependencies
)

# Add a custom command to copy shader files to the build directory
# This assumes vert.spv and frag.spv are in the src/ directory next to main.cpp
# and will be copied to the location of the VaporFrameEngine executable.
if(VULKAN_AVAILABLE)
    if(WIN32)
    add_custom_command(
        TARGET VaporFrameEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/vert.spv"
            "$<TARGET_FILE_DIR:VaporFrameEngine>/vert.spv"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/frag.spv"
            "$<TARGET_FILE_DIR:VaporFrameEngine>/frag.spv"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:VaporFrameEngine>/shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_SOURCE_DIR}/shaders/ui.vert.spv"
            "$<TARGET_FILE_DIR:VaporFrameEngine>/shaders/ui.vert.spv"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_SOURCE_DIR}/shaders/ui.frag.spv"
            "$<TARGET_FILE_DIR:VaporFrameEngine>/shaders/ui.frag.spv"
        COMMENT "Copying shaders to executable directory"
    )
elseif(APPLE)
    # Similar command for macOS if needed, adjust paths
    add_custom_command(
        TARGET VaporFrameEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/vert.spv"
            "$<TARGET_FILE_DIR:VaporFrameEngine>/../Resources/vert.spv" # Example for .app bundle
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/frag.spv"
            "$<TARGET_FILE_DIR:VaporFrameEngine>/../Resources/frag.spv"
        COMMENT "Copying shaders to app bundle"
    )
else()
    # Linux and other platforms
    add_custom_command(
        TARGET VaporFrameEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/vert.spv"
            "$<TARGET_FILE_DIR:VaporFrameEngine>/vert.spv"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/frag.spv"
            "$<TARGET_FILE_DIR:VaporFrameEngine>/frag.spv"
        COMMENT "Copying shaders to executable directory"
    )
endif()
endif()

# We might want to create a simple executable target here for initial testing
# add_executable(EngineTest main.cpp)
# target_link_libraries(EngineTest VaporFrameCore glfw) 