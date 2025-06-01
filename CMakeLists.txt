cmake_minimum_required(VERSION 3.16)
project(VaporFrameEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include FetchContent for managing external dependencies
include(FetchContent)

# Fetch GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1 # Updated to a newer stable tag
)
FetchContent_MakeAvailable(glm)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for CMake GUI
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Vulkan
find_package(Vulkan REQUIRED)

# Add GLFW from third_party directory
# Ensure you have downloaded GLFW and placed it in third_party/glfw
add_subdirectory(third_party/glfw-3.4 EXCLUDE_FROM_ALL)

add_subdirectory(src)
# add_subdirectory(editor) # We'll add this later
# add_subdirectory(projects) # For example projects

message(STATUS "Using C++ Standard: ${CMAKE_CXX_STANDARD}") 