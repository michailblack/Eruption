cmake_minimum_required(VERSION 3.30)

project(Eruption)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "Available build configurations" FORCE)

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

# Add option to enable/disable C++ 20 module
option(ENABLE_CPP20_MODULE "Enable C++ 20 module support for Vulkan" OFF)

# Enable C++ module dependency scanning only if C++ 20 module is enabled
if (ENABLE_CPP20_MODULE)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif ()

# Dependencies
find_package(spdlog REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan 1.4.335 REQUIRED)

# Set up Vulkan C++ module only if enabled
if (ENABLE_CPP20_MODULE)
    add_library(VulkanCppModule)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)

    target_compile_definitions(VulkanCppModule
            PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
    target_include_directories(VulkanCppModule
            PUBLIC
            "${Vulkan_INCLUDE_DIR}"
    )
    target_link_libraries(VulkanCppModule
            PUBLIC
            Vulkan::Vulkan
    )

    set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

    # Add MSVC-specific compiler options for proper C++ module support
    if (MSVC)
        target_compile_options(VulkanCppModule PRIVATE
                /std:c++latest      # Use latest C++ standard for better module support
                /permissive-        # Standards conformance mode
                /Zc:__cplusplus     # Enable correct __cplusplus macro
                /EHsc               # Enable C++ exception handling
                /Zc:preprocessor    # Use conforming preprocessor
                /translateInclude   # Automatically translate #include to import for standard library
        )
    endif ()

    target_sources(VulkanCppModule
            PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES
            BASE_DIRS
            "${Vulkan_INCLUDE_DIR}"
            FILES
            "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )


    # Add the vulkan.cppm file directly as a source file
    target_sources(VulkanCppModule
            PRIVATE
            "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )
else ()
    # Create a dummy interface library when C++ 20 module is disabled
    add_library(VulkanCppModule INTERFACE)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)
    target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
    target_compile_definitions(VulkanCppModule
            INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
endif ()

add_subdirectory(Eruption)
add_subdirectory(Editor)