cmake_minimum_required(VERSION 3.30)

project(Eruption-Core VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h")

add_executable(Eruption-Core
    ${CORE_SOURCES}
)

if(CPPCHECK_EXECUTABLE)
  set_target_properties(Eruption-Core PROPERTIES
      CMAKE_CXX_CPPCHECK "${CPPCHECK_EXECUTABLE}"
  )
endif()

target_include_directories(Eruption-Core PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    "${VENDOR_DIR}/GLFW/include"
    "${VENDOR_DIR}/GLM"
    "${VENDOR_DIR}/stb"
)

target_link_libraries(Eruption-Core PRIVATE
    Vulkan::Vulkan
    glfw
)

set_target_properties(Eruption-Core PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/${PROJECT_NAME}"
    LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}/${PROJECT_NAME}"
    ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}/${PROJECT_NAME}"
)

# MSVC-specific runtime settings (dynamic runtime, as staticruntime was "off")
if(MSVC)
  target_compile_options(Eruption-Core PRIVATE
        $<$<CONFIG:Debug>:/MDd>
        $<$<CONFIG:Release>:/MD>
        $<$<CONFIG:Dist>:/MD>
    )
endif()

# Set preprocessor definitions based on configuration
target_compile_definitions(Eruption-Core PRIVATE
    $<$<CONFIG:Debug>:ER_DEBUG>
    $<$<CONFIG:Release>:ER_RELEASE>
    $<$<CONFIG:Dist>:ER_DIST>
)
