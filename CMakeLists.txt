cmake_minimum_required(VERSION 3.16)
project(VaporFrameEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project configuration
option(VAPORFRAME_ENABLE_VALIDATION "Enable Vulkan validation layers" ON)
option(VAPORFRAME_ENABLE_TESTS "Enable unit tests" OFF)
option(VAPORFRAME_ENABLE_DOCS "Enable documentation generation" OFF)

# Include FetchContent for managing external dependencies
include(FetchContent)

# Fetch GLM (Mathematics library)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# Fetch spdlog (Logging library)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# Fetch yaml-cpp (YAML parsing)
# FetchContent_Declare(
#   yaml-cpp
#   GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
#   GIT_TAG 0.7.0
# )
# FetchContent_MakeAvailable(yaml-cpp)

# Fetch mimalloc (High-performance allocator)
FetchContent_Declare(
  mimalloc
  GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
  GIT_TAG v2.1.7
)
FetchContent_MakeAvailable(mimalloc)

# Fetch Dear ImGui (UI library)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# WebView2 is available on Windows 10+ and doesn't need to be fetched
# We'll use the system WebView2 runtime

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for CMake GUI
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Vulkan - Make it optional for CI/CD environments
find_package(Vulkan QUIET)
if(NOT Vulkan_FOUND)
    message(WARNING "Vulkan not found. This is expected in CI/CD environments without Vulkan support.")
    message(WARNING "The engine will not be able to build without Vulkan. Please install Vulkan SDK for development.")
    # Set a flag to indicate Vulkan is not available
    set(VULKAN_AVAILABLE FALSE)
else()
    set(VULKAN_AVAILABLE TRUE)
    message(STATUS "Vulkan found: ${Vulkan_VERSION}")
endif()

# Add GLFW from third_party directory
# Ensure you have downloaded GLFW and placed it in third_party/glfw
add_subdirectory(third_party/glfw-3.4 EXCLUDE_FROM_ALL)

# Add subdirectories
add_subdirectory(src)

# Conditionally add test directory
if(VAPORFRAME_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Conditionally add documentation
if(VAPORFRAME_ENABLE_DOCS)
    add_subdirectory(docs)
endif()

# Print configuration summary
message(STATUS "VaporFrame Engine Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Validation Layers: ${VAPORFRAME_ENABLE_VALIDATION}")
message(STATUS "  Tests Enabled: ${VAPORFRAME_ENABLE_TESTS}")
message(STATUS "  Documentation: ${VAPORFRAME_ENABLE_DOCS}") 