message(STATUS "[GRACE_INFO] Building Grace...")

include(FetchContent)

FetchContent_Declare(
    pybind
    URL https://github.com/pybind/pybind11/archive/refs/tags/v3.0.1.zip
)

FetchContent_Declare(
    vma
    URL https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.3.0.zip
)
FetchContent_MakeAvailable(vma pybind)

if (NOT DEFINED GRACE_TARGET_VULKAN_API_VERSION)
    message(STATUS "[GRACE_WARN] GRACE_TARGET_VULKAN_API_VERSION was not defined - default is 1.3!")
    set(GRACE_TARGET_VULKAN_API_VERSION 13)
endif()

if(NOT DEFINED GRACE_SHADERS_DIR)
    message(FATAL_ERROR "[GRACE_ERROR] GRACE_SHADERS_DIR must be defined!")
endif()

if(NOT DEFINED GRACE_SPIRV_DIR)
    message(STATUS "[GRACE_WARN] GRACE_SPIRV_DIR was not defined - falling back to default!")
    set(GRACE_SPIRV_DIR ${GRACE_SHADERS_DIR}/../SPIRV)
endif()

include(${PROJECT_SOURCE_DIR}/CMake/GraceFindVulkan.cmake)

add_library(Grace SHARED)

set(GRACE_PY_SHADER_COMP_SCRIPT ${PROJECT_SOURCE_DIR}/Scripts/ShaderCompiler.py)

if(GRACE_ENABLE_RUNTIME_SHADER_COMP_EXT)
    target_sources(Grace PUBLIC Grace/Ext/ShaderCompiler.cpp)
    target_link_libraries(Grace PUBLIC pybind11::embed)
    target_compile_definitions(Grace PUBLIC GRACE_PY_SHADER_COMP_SCRIPT="${GRACE_PY_SHADER_COMP_SCRIPT}")
endif()

set(GRACE_LIB_SOURCE
    Grace/Buffer.cpp
    Grace/CommandGroup.cpp
    Grace/Context.cpp
    Grace/DebugReporter.cpp
    Grace/DeletionQueue.cpp
    Grace/DescriptorGroup.cpp
    Grace/Device.cpp
    Grace/Fence.cpp
    Grace/GpuResourceTable.cpp
    Grace/HelperFunctions.cpp
    Grace/Image.cpp
    Grace/PipelineGroup.cpp
    Grace/QueryManager.cpp
    Grace/ResourceManager.cpp
    Grace/Sampler.cpp
    Grace/Semaphore.cpp
    Grace/Swapchain.cpp
    Grace/SyncGroup.cpp
    Grace/VmaUsage.cpp
)

target_sources(
    Grace
    PUBLIC ${GRACE_LIB_SOURCE}
)

target_compile_features(
    Grace
    PUBLIC cxx_std_20
)

set_target_properties(
    Grace PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    C_VISIBILITY_PRESET "hidden"
    CXX_VISIBILITY_PRESET "hidden"
    VISIBILITY_INLINES_HIDDEN TRUE
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include(GenerateExportHeader)
generate_export_header(
    Grace
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/Grace/GraceExport.h"
)

target_include_directories(
    Grace
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${Vulkan_INCLUDE_DIRS}
    PUBLIC ${vma_SOURCE_DIR}/include/
)

target_link_libraries(
    Grace
    PUBLIC ${Vulkan_LIBRARIES}
)

include(${PROJECT_SOURCE_DIR}/CMake/ShaderCompilation.cmake)

target_compile_definitions(
    Grace
    PUBLIC Grace_EXPORTS
    PUBLIC GRACE_SHADERS_DIR="${GRACE_SHADERS_DIR}"
    PUBLIC GRACE_SPIRV_DIR="${GRACE_SPIRV_DIR}"
    PUBLIC GRACE_SLANG_COMPILER="${SLANG_COMPILER}"
    PUBLIC GRACE_SLANG_COMPILER_ARGS="${GRACE_SLANG_COMPILER_ARGS}"
    PUBLIC GRACE_HLSL_COMPILER="${HLSL_COMPILER}"
    PUBLIC GRACE_HLSL_COMPILER_ARGS="${GRACE_HLSL_COMPILER_ARGS}"
    PUBLIC GRACE_GLSL_COMPILER="${GLSL_COMPILER}"
    PUBLIC GRACE_GLSL_COMPILER_ARGS="${GRACE_GLSL_COMPILER_ARGS}"
    PUBLIC GRACE_TARGET_VULKAN_API_VERSION=${GRACE_TARGET_VULKAN_API_VERSION}
)
