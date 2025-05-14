cmake_minimum_required(VERSION 3.30)

project(Eruption)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "Available build configurations" FORCE)

if(MSVC)
  add_compile_options(/EHsc /Zc:preprocessor /Zc:__cplusplus)
endif()

# Set output directories
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/Binaries")

# Attempt to find cppcheck for static analysis
find_program(CPPCHECK_EXECUTABLE cppcheck)
if(CPPCHECK_EXECUTABLE)
  message(STATUS "Cppcheck found: ${CPPCHECK_EXECUTABLE}")
  set(CPPCHECK_COMMAND "cppcheck;--enable=warning,style,performance,portability;--inconclusive;--verbose;--check-level=exhaustive;--language=c++;--std=c++23")
else()
  message(WARNING "Cppcheck not found. Static analysis will be skipped.")
endif()

# Dependencies
find_package(Vulkan REQUIRED)

# Add vendor libraries
set(VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Vendor")

set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
if(UNIX AND NOT APPLE)
  set(GLFW_BUILD_WAYLAND ON)
  set(GLFW_BUILD_X11 OFF)
endif()
add_subdirectory("${VENDOR_DIR}/GLFW")

set(GLM_ENABLE_CXX_20 ON)
add_subdirectory("${VENDOR_DIR}/GLM")

# Add subprojects
add_subdirectory(Eruption-Core)
