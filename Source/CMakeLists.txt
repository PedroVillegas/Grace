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
endif ()

if (NOT DEFINED GRACE_SHADERS_DIR)
    message(FATAL_ERROR "[GRACE_ERROR] GRACE_SHADERS_DIR must be defined!")
endif ()

if (NOT DEFINED GRACE_SPIRV_DIR)
    message(STATUS "[GRACE_WARN] GRACE_SPIRV_DIR was not defined - falling back to default!")
    set(GRACE_SPIRV_DIR ${GRACE_SHADERS_DIR}/../SPIRV)
endif ()

include(${PROJECT_SOURCE_DIR}/CMake/GraceFindVulkan.cmake)

add_library(Grace SHARED)

set(GRACE_PY_SHADER_COMP_SCRIPT ${PROJECT_SOURCE_DIR}/Scripts/ShaderCompiler.py)

if (GRACE_ENABLE_RUNTIME_SHADER_COMP_EXT)
    target_sources(Grace PRIVATE Private/Grace/ShaderCompiler.cpp)
    target_link_libraries(Grace PUBLIC pybind11::embed)
    target_compile_definitions(Grace PUBLIC GRACE_PY_SHADER_COMP_SCRIPT="${GRACE_PY_SHADER_COMP_SCRIPT}")
endif ()

target_sources(
    Grace
        PRIVATE
            Private/Grace/PipelineGroup.cpp
            Private/Grace/Buffer.cpp
            Private/Grace/CommandGroup.cpp
            Private/Grace/Context.cpp
            Private/Grace/DebugReporter.cpp
            Private/Grace/DeletionQueue.cpp
            Private/Grace/DescriptorGroup.cpp
            Private/Grace/Device.cpp
            Private/Grace/Fence.cpp
            Private/Grace/GpuResourceTable.cpp
            Private/Grace/HelperFunctions.cpp
            Private/Grace/Image.cpp
            Private/Grace/QueryManager.cpp
            Private/Grace/ResourceManager.cpp
            Private/Grace/Sampler.cpp
            Private/Grace/Semaphore.cpp
            Private/Grace/Swapchain.cpp
            Private/Grace/SyncGroup.cpp
            Private/Grace/VmaUsage.cpp
	        Private/Grace/StackAllocator.cpp
)

target_compile_features(
    Grace
        PUBLIC
            cxx_std_20
)

set_target_properties(
    Grace
        PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            C_VISIBILITY_PRESET "hidden"
            CXX_VISIBILITY_PRESET "hidden"
            VISIBILITY_INLINES_HIDDEN OFF
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
)

include(GenerateExportHeader)
set(content [[

#ifdef GRACE_STATIC_DEFINE
#  define GRACE_API_TESTBED
#else
#  ifndef GRACE_API_TESTBED
#    ifdef Grace_EXPORTS
        /* We are building this library */
#      define GRACE_API_TESTBED __declspec(dllexport)
#    else
        /* We are using this library */
#      define GRACE_API_TESTBED __declspec(dllimport)
#    endif
#  endif

#  ifndef GRACE_NO_API
#    define GRACE_NO_API
#  endif
#endif

]])
generate_export_header(
    Grace
    EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/Grace/GraceApi.hpp"
    EXPORT_MACRO_NAME "GRACE_API"
    NO_EXPORT_MACRO_NAME "GRACE_NO_API"
    CUSTOM_CONTENT_FROM_VARIABLE content
)

target_include_directories(
    Grace
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/Public/
            ${Vulkan_INCLUDE_DIRS}
            ${vma_SOURCE_DIR}/include/
            ${CMAKE_CURRENT_BINARY_DIR}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    Grace
        PUBLIC
            ${Vulkan_LIBRARIES}
)

include(${PROJECT_SOURCE_DIR}/CMake/ShaderCompilation.cmake)

target_compile_definitions(
    Grace
        PUBLIC
            Grace_EXPORTS
            GRACE_SHADERS_DIR="${GRACE_SHADERS_DIR}"
            GRACE_SPIRV_DIR="${GRACE_SPIRV_DIR}"
            GRACE_SLANG_COMPILER="${SLANG_COMPILER}"
            GRACE_SLANG_COMPILER_ARGS="${GRACE_SLANG_COMPILER_ARGS}"
            GRACE_HLSL_COMPILER="${HLSL_COMPILER}"
            GRACE_HLSL_COMPILER_ARGS="${GRACE_HLSL_COMPILER_ARGS}"
            GRACE_GLSL_COMPILER="${GLSL_COMPILER}"
            GRACE_GLSL_COMPILER_ARGS="${GRACE_GLSL_COMPILER_ARGS}"
            GRACE_TARGET_VULKAN_API_VERSION=${GRACE_TARGET_VULKAN_API_VERSION}
)

if (GRACE_USE_GLFW)
    FetchContent_Declare(
        glfw
        EXCLUDE_FROM_ALL
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
    )
    FetchContent_MakeAvailable(glfw)

    target_link_libraries(Grace PUBLIC glfw)
    target_compile_definitions(Grace PRIVATE GRACE_USE_GLFW)
endif ()
