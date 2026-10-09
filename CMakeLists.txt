cmake_minimum_required(VERSION 3.30)

project(Eruption LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/Binaries")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

if(WIN32)
  add_compile_definitions(ER_PLATFORM_WINDOWS)
  message(STATUS "Platform: Windows")
elseif(UNIX AND NOT APPLE)
  add_compile_definitions(ER_PLATFORM_LINUX)
  message(STATUS "Platform: Linux")
elseif(APPLE)
  add_compile_definitions(ER_PLATFORM_MACOS)
  message(STATUS "Platform: macOS")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_definitions(ER_COMPILER_CLANG)
  message(STATUS "Compiler: Clang (${CMAKE_CXX_COMPILER_VERSION})")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_definitions(ER_COMPILER_GCC)
  message(STATUS "Compiler: GCC (${CMAKE_CXX_COMPILER_VERSION})")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_definitions(ER_COMPILER_MSVC)
  add_compile_options(/EHsc /Zc:preprocessor /Zc:__cplusplus)
  message(STATUS "Compiler: MSVC (${CMAKE_CXX_COMPILER_VERSION})")
endif()

get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(IS_MULTI_CONFIG)
  set(CMAKE_CONFIGURATION_TYPES
      "Debug;Release;Dist"
      CACHE STRING "Available build configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Debug
      CACHE STRING "Build type" FORCE)
endif()

add_compile_definitions($<$<CONFIG:Debug>:ER_DEBUG> $<$<CONFIG:Release>:ER_RELEASE> $<$<CONFIG:Dist>:ER_DIST>)

option(ER_ENABLE_CPPCHECK "Run cppcheck as part of the build" OFF)
if(ER_ENABLE_CPPCHECK)
  find_program(CPPCHECK_EXECUTABLE cppcheck)
  if(CPPCHECK_EXECUTABLE)
    message(STATUS "Found Cppcheck: ${CPPCHECK_EXECUTABLE}")
    set(CMAKE_CXX_CPPCHECK
        "${CPPCHECK_EXECUTABLE};--quiet;--enable=warning,performance,portability;--suppress=uninitMemberVarNoCtor")
  else()
    message(WARNING "ER_ENABLE_CPPCHECK is ON but cppcheck was not found.")
  endif()
endif()

option(ENABLE_CPP20_MODULE "Enable C++20 module support for Vulkan-Hpp" OFF)

# TODO: Use the packages as submodules
find_package(Vulkan 1.3 REQUIRED)
find_package(slang REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)

add_subdirectory(Vendor/entt)

add_library(stb INTERFACE)
add_library(Vendor::stb ALIAS stb)
target_include_directories(stb SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/stb")

if(ENABLE_CPP20_MODULE)
  add_library(VulkanCppModule STATIC)
  add_library(Vulkan::cppm ALIAS VulkanCppModule)
  target_compile_definitions(VulkanCppModule PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
                                                    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)
  target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)
  set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)
  target_sources(
    VulkanCppModule
    PUBLIC FILE_SET
           cxx_modules
           TYPE
           CXX_MODULES
           BASE_DIRS
           "${Vulkan_INCLUDE_DIR}"
           FILES
           "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm")
else()
  add_library(VulkanCppModule INTERFACE)
  add_library(Vulkan::cppm ALIAS VulkanCppModule)
  target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
  target_compile_definitions(VulkanCppModule INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
                                                       VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)
endif()

add_subdirectory(Eruption)
add_subdirectory(Editor)
