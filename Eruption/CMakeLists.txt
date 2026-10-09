project(
  Eruption-Core
  VERSION 1.0
  LANGUAGES C CXX)

find_package(Threads REQUIRED)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp")

add_library(${PROJECT_NAME} SHARED ${CORE_SOURCES})
add_library(Eruption::Core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC Vulkan::cppm glm::glm glfw spdlog::spdlog EnTT::EnTT Threads::Threads
  PRIVATE slang::slang assimp::assimp Vendor::stb)

target_compile_definitions(${PROJECT_NAME} PUBLIC GLFW_INCLUDE_NONE)

target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS GLM_ENABLE_EXPERIMENTAL)

target_compile_definitions(${PROJECT_NAME} PUBLIC ER_SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders"
                                                  ER_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets")

target_precompile_headers(${PROJECT_NAME} PRIVATE "Source/erpch.h")

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}" LIBRARY_OUTPUT_DIRECTORY
                                                                                          "${OUTPUT_DIR}")
