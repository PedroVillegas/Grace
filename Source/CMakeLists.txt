message(STATUS "[GRACE_INFO] Building Grace...")

include(FetchContent)

FetchContent_Declare(
        vma
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.3.0.zip
)
FetchContent_MakeAvailable(vma)

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

include(${CMAKE_SOURCE_DIR}/CMake/GraceFindVulkan.cmake)

add_library(Grace SHARED)

set(GRACE_LIB_SOURCE
    Grace/Buffer.cpp
    Grace/CommandGroup.cpp
    Grace/Context.cpp
    Grace/DebugReporter.cpp
    Grace/DescriptorGroup.cpp
    Grace/Device.cpp
    Grace/DeletionQueue.cpp
    Grace/GpuResourceTable.cpp
    Grace/HelperFunctions.cpp
    Grace/Image.cpp
    Grace/PipelineGroup.cpp
    Grace/ResourceManager.cpp
    Grace/Sampler.cpp
    Grace/Swapchain.cpp
    Grace/SyncGroup.cpp
    Grace/QueryManager.cpp
    Grace/Fence.cpp
    Grace/Semaphore.cpp
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

target_compile_definitions(
    Grace
    PUBLIC Grace_EXPORTS
    PUBLIC GRACE_SPIRV_DIR="${GRACE_SPIRV_DIR}"
    PUBLIC GRACE_TARGET_VULKAN_API_VERSION=${GRACE_TARGET_VULKAN_API_VERSION}
)

include(${CMAKE_SOURCE_DIR}/CMake/ShaderCompilation.cmake)
