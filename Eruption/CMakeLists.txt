cmake_minimum_required(VERSION 3.30)

project(Eruption-Core VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Source/Eruption/*.cpp")

add_library(${PROJECT_NAME} SHARED ${CORE_SOURCES})

if (CPPCHECK_EXECUTABLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            CMAKE_CXX_CPPCHECK "${CPPCHECK_EXECUTABLE}"
    )
endif ()

target_include_directories(${PROJECT_NAME}
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        PRIVATE "${VENDOR_DIR}/GLFW/include"
        PRIVATE "${VENDOR_DIR}/GLM"
        PRIVATE "${VENDOR_DIR}/stb"
        PRIVATE "${VENDOR_DIR}/VulkanMemoryAllocator/src" # For VmaUsage.h
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC spdlog::spdlog
        PRIVATE glfw
        PRIVATE Vulkan::Vulkan
        PRIVATE GPUOpen::VulkanMemoryAllocator
)

target_precompile_headers(${PROJECT_NAME} PRIVATE "Source/erpch.h")

set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/${PROJECT_NAME}"
        LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}/${PROJECT_NAME}"
        ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}/${PROJECT_NAME}"
)

# MSVC-specific runtime settings (dynamic runtime, as staticruntime was "off")
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:/MDd>
            $<$<CONFIG:Release>:/MD>
            $<$<CONFIG:Dist>:/MD>
    )
endif ()

# Set preprocessor definitions based on configuration
target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:ER_DEBUG>
        $<$<CONFIG:Release>:ER_RELEASE>
        $<$<CONFIG:Dist>:ER_DIST>
)
