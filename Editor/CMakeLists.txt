project(
  Eruption-Editor
  VERSION 1.0
  LANGUAGES CXX)

file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp")

add_executable(${PROJECT_NAME} ${EDITOR_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_link_libraries(${PROJECT_NAME} PRIVATE Eruption::Core)

target_precompile_headers(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../Eruption/Source/erpch.h")

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
             BUILD_RPATH "$ORIGIN"
             INSTALL_RPATH "$ORIGIN")
