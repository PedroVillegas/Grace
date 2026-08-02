message(STATUS "[GRACE_INFO] Building Grace...")

include(FetchContent)

FetchContent_Declare(
	pybind
	URL https://github.com/pybind/pybind11/archive/refs/tags/v3.0.1.zip
)

FetchContent_Declare(
	yaml-cpp
	GIT_REPOSITORY 	https://github.com/jbeder/yaml-cpp.git
	GIT_TAG 		yaml-cpp-0.9.0
)

FetchContent_Declare(
	spdlog
	GIT_REPOSITORY 	https://github.com/gabime/spdlog.git
	GIT_TAG			v1.17.0
)

FetchContent_Declare(
	vma
	URL https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.3.0.zip
)

FetchContent_MakeAvailable(vma pybind yaml-cpp spdlog)

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

include(GraceFindVulkan)

add_library(Grace SHARED)

set(Third-Party ${PROJECT_SOURCE_DIR}/Third-Party)
set(Third-Party-Sources "")
define_property(TARGET PROPERTY THIRD_PARTY_SOURCES)

set(GRACE_PY_SHADER_COMP_SCRIPT ${PROJECT_SOURCE_DIR}/Scripts/ShaderCompiler.py)
set(GRACE_PY_YAML_CONFIG_PARSER_SCRIPT ${PROJECT_SOURCE_DIR}/Scripts/YamlConfigParser.py)

if (GRACE_ENABLE_RUNTIME_SHADER_COMP_EXT)
	target_sources(Grace PRIVATE
		Grace/Ext/ShaderCompiler.cpp
		Grace/Ext/ImGrace.cpp
	)
	target_include_directories(Grace PUBLIC ${Third-Party})
	target_link_libraries(Grace PUBLIC pybind11::embed glfw)
	target_compile_definitions(Grace PUBLIC GRACE_PY_SHADER_COMP_SCRIPT="${GRACE_PY_SHADER_COMP_SCRIPT}")
endif()

if (GRACE_ENABLE_IMGUI_EXT)
	list(APPEND Third-Party-Sources
		${Third-Party}/imgui/backends/imgui_impl_glfw.cpp
		${Third-Party}/imgui/imgui.cpp
		${Third-Party}/imgui/imgui_demo.cpp
		${Third-Party}/imgui/imgui_draw.cpp
		${Third-Party}/imgui/imgui_tables.cpp
		${Third-Party}/imgui/imgui_widgets.cpp
	)
	target_sources(Grace PUBLIC
		${Third-Party-Sources}
	)
endif ()

target_sources(Grace PRIVATE
	Grace/Detail/Config.cpp
	Grace/Detail/StackAllocator.cpp
	Grace/Detail/SyncGroup.cpp
	Grace/GpuBuffer.cpp
	Grace/CommandGroup.cpp
	Grace/Context.cpp
	Grace/DebugReporter.cpp
	Grace/DeletionQueue.cpp
	Grace/DescriptorGroup.cpp
	Grace/Device.cpp
	Grace/GpuFence.cpp
	Grace/GpuHandle.cpp
	Grace/GpuBindlessTable.cpp
	Grace/HelperFunctions.cpp
	Grace/GpuImage.cpp
	Grace/GpuPipelineGroup.cpp
	Grace/QueryManager.cpp
	Grace/GpuSampler.cpp
	Grace/GpuSemaphore.cpp
	Grace/Swapchain.cpp
	Grace/VmaUsage.cpp
)

target_compile_features(Grace PUBLIC cxx_std_20)

set_target_properties(Grace PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	C_VISIBILITY_PRESET "hidden"
	CXX_VISIBILITY_PRESET "hidden"
	VISIBILITY_INLINES_HIDDEN ON
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION}
)

include(GenerateExportHeader)
generate_export_header(Grace
	EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/Grace/GraceApi.hpp"
	EXPORT_MACRO_NAME "GRACE_API"
	NO_EXPORT_MACRO_NAME "GRACE_NO_API"
	CUSTOM_CONTENT_FROM_VARIABLE content
)

target_include_directories(Grace PUBLIC
	${Vulkan_INCLUDE_DIRS}
	${vma_SOURCE_DIR}/include/
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Grace PUBLIC
	${Vulkan_LIBRARIES}
	yaml-cpp
	spdlog
)

include(ShaderCompilation)

target_compile_definitions(Grace PUBLIC
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
	GRACE_INTERNAL_SPV="${PROJECT_SOURCE_DIR}/Source/Shaders"
)

set_property(TARGET Grace PROPERTY THIRD_PARTY_SOURCES "${Third-Party-Sources}")
