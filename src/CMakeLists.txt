# We'll change VaporFrameCore to be an executable for now to test main.cpp
add_executable(VaporFrameEngine main.cpp) # Renamed target to VaporFrameEngine

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
    PRIVATE
        # Any private link dependencies
)

# We might want to create a simple executable target here for initial testing
# add_executable(EngineTest main.cpp)
# target_link_libraries(EngineTest VaporFrameCore glfw) 