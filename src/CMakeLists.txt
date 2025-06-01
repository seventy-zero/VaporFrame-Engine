# We'll change VaporFrameCore to be an executable for now to test main.cpp
add_executable(VaporFrameEngine
    main.cpp
    VulkanRenderer.cpp
    # VulkanRenderer.h # Headers are usually not listed here but found via include paths
)

# target_sources(VaporFrameCore
# PRIVATE
# main.cpp
#         # Add core engine source files here later
#         # Example: MainLoop.cpp, Window.cpp, etc.
# )

target_include_directories(VaporFrameEngine # Changed target name
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR} # For includes within src
        ${PROJECT_SOURCE_DIR}/third_party/glfw-3.4/include # To find <GLFW/glfw3.h>
    PRIVATE
        # Any private include directories for the core
)

target_link_libraries(VaporFrameEngine # Changed target name
    PUBLIC
        glfw # Link against the glfw target made available by add_subdirectory
        Vulkan::Vulkan # Link against Vulkan
        glm::glm # Link against GLM
    PRIVATE
        # Any private link dependencies
)

# Add a custom command to copy shader files to the build directory
# This assumes vert.spv and frag.spv are in the src/ directory next to main.cpp
# and will be copied to the location of the VaporFrameEngine executable.
if(WIN32)
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

# We might want to create a simple executable target here for initial testing
# add_executable(EngineTest main.cpp)
# target_link_libraries(EngineTest VaporFrameCore glfw) 