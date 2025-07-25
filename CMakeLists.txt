cmake_minimum_required(VERSION 3.30)

project(Eruption)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "Available build configurations" FORCE)

# Set output directories
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/Binaries")

# Check platform
if (WIN32)
    add_compile_definitions(ER_PLATFORM_WINDOWS)
    message(STATUS "Platform: Windows")
elseif (UNIX AND NOT APPLE)
    add_compile_definitions(ER_PLATFORM_LINUX)
    message(STATUS "Platform: Linux")
elseif (APPLE)
    add_compile_definitions(ER_PLATFORM_MACOS)
    message(STATUS "Platform: macOS")
endif ()

# Check compiler
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    add_compile_definitions(ER_COMPILER_CLANG)
    message(STATUS "Compiler: Clang (${CMAKE_CXX_COMPILER_VERSION})")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_definitions(ER_COMPILER_GCC)
    message(STATUS "Compiler: GCC (${CMAKE_CXX_COMPILER_VERSION})")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_definitions(ER_COMPILER_MSVC)
    add_compile_options(/EHsc /Zc:preprocessor /Zc:__cplusplus)
    message(STATUS "Compiler: MSVC (${CMAKE_CXX_COMPILER_VERSION})")
endif ()

# Check build type
string(TOUPPER "${CMAKE_BUILD_TYPE}" UPPERCASE_BUILD_TYPE)
if (UPPERCASE_BUILD_TYPE STREQUAL "DEBUG")
    add_compile_definitions(ER_DEBUG)
    message(STATUS "Build: Debug")
else ()
    add_compile_definitions(ER_RELEASE)
    message(STATUS "Build: Release")
endif ()

# For multi-configuration generators like Visual Studio
# that don't use CMAKE_BUILD_TYPE
if (CMAKE_CONFIGURATION_TYPES)
    add_compile_definitions(
            $<$<CONFIG:Debug>:ER_DEBUG>
            $<$<NOT:$<CONFIG:Debug>>:ER_RELEASE>
    )
endif ()

# Attempt to find cppcheck for static analysis
find_program(CPPCHECK_EXECUTABLE cppcheck)
if (CPPCHECK_EXECUTABLE)
    message(STATUS "Cppcheck found: ${CPPCHECK_EXECUTABLE}")
    set(CPPCHECK_COMMAND "cppcheck;--enable=warning,style,performance,portability;--inconclusive;--verbose;--check-level=exhaustive;--language=c++;--std=c++23")
else ()
    message(WARNING "Cppcheck not found. Static analysis will be skipped.")
endif ()

# Dependencies
find_package(Vulkan REQUIRED)

# Add vendor libraries
set(VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Vendor")

# Add GLFW
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
if (UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND ON)
    set(GLFW_BUILD_X11 OFF)
endif ()
add_subdirectory("${VENDOR_DIR}/GLFW")

# Add GLM
set(GLM_BUILD_LIBRARY ON)
set(GLM_ENABLE_CXX_20 ON)
add_subdirectory("${VENDOR_DIR}/GLM")

# Add spdlog
set(SPDLOG_BUILD_SHARED ON)
set(SPDLOG_ENABLE_PCH ON)
set(SPDLOG_USE_STD_FORMAT ON)
set(SPDLOG_DISABLE_DEFAULT_LOGGER ON)
add_subdirectory("${VENDOR_DIR}/spdlog")

# Add VulkanMemoryAllocator
add_subdirectory("${VENDOR_DIR}/VulkanMemoryAllocator")

# Add subprojects
add_subdirectory(Eruption)
add_subdirectory(Editor)
