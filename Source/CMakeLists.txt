include(FetchContent)

FetchContent_Declare(
        vma
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.2.1.zip
)

FetchContent_MakeAvailable(vma)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Grace/*.cpp")
set(GRACE_LIB_SOURCE ${SOURCE_FILES} "${vma_SOURCE_DIR}/src/VmaUsage.cpp")

set(VULKAN_PATH "C:/VulkanSDK/1.4.313.0")
if (EXISTS ${VULKAN_PATH})
    message("-- Found Vulkan SDK: ${VULKAN_PATH}")
    set(VULKAN_HEADER "${VULKAN_PATH}/Include")
    set(VULKAN_LIB "${VULKAN_PATH}/Lib/vulkan-1.lib")
endif()

set_target_properties(Grace PROPERTIES OUTPUT_NAME Grace)

target_sources(Grace PUBLIC ${GRACE_LIB_SOURCE})

target_include_directories(Grace PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${VULKAN_HEADER})

target_link_libraries(Grace VulkanMemoryAllocator ${VULKAN_LIB})

target_compile_definitions(Grace PUBLIC PATH_TO_SPIRV="${SPIRV_DIR}")

# Compile glsl shaders into spv
find_program(GLSL_VALIDATOR glslc HINTS ${VULKAN_PATH}/Bin/)
if (EXISTS ${GLSL_VALIDATOR})
    message("-- Found GLSL to SPIRV compiler (${GLSL_VALIDATOR})")
endif()

find_program(HLSL_COMPILER dxc HINTS ${VULKAN_PATH}/Bin/)
if (EXISTS ${HLSL_COMPILER})
    message("-- Found HLSL to SPIRV compiler (${HLSL_COMPILER})")
endif()

#find_program(SLANG_COMPILER slangc HINTS ${VULKAN_PATH}/Bin/)
set(SLANG_COMPILER "${CMAKE_CURRENT_SOURCE_DIR}/Bin/slang-2025.8.1/bin/slangc.exe")
if (EXISTS ${SLANG_COMPILER})
    message("-- Found SLANG to SPIRV compiler (${SLANG_COMPILER})")
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${SHADERS_DIR}/*.frag"
        "${SHADERS_DIR}/*.vert"
        "${SHADERS_DIR}/*.comp"
)

file(GLOB_RECURSE HLSL_FILES
        "${SHADERS_DIR}/*.hlsl"
)

file(GLOB_RECURSE SLANG_FILES
        "${SHADERS_DIR}/*.slang"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${SPIRV_DIR}/${FILE_NAME}.spv")
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} ${GLSL} -g -o ${SPIRV} --target-env=vulkan1.4
            DEPENDS ${GLSL}
            COMMENT "Compiling GLSL Shader: ${GLSL}"
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

foreach(HLSL ${HLSL_FILES})
    # Read top of file to determine shader stage
    file(STRINGS ${HLSL} SHADER_STAGE LIMIT_INPUT 32 REGEX "#pragma")
    if (${SHADER_STAGE} STREQUAL "#pragma compute")
        set(TARGET_PROFILE "cs_6_5")
    elseif(${SHADER_STAGE} STREQUAL "#pragma vertex")
        set(TARGET_PROFILE "vs_6_5")
    elseif(${SHADER_STAGE} STREQUAL "#pragma pixel")
        set(TARGET_PROFILE "ps_6_5")
    endif()

    get_filename_component(FILE_NAME ${HLSL} NAME)
    set(SPIRV "${SPIRV_DIR}/${FILE_NAME}.spv")

    set(HLSL_COMPILE_CMD
            ${VULKAN_PATH}/Bin/dxc.exe ${HLSL}
            -spirv
            -HV 2021
            -T ${TARGET_PROFILE}
            -E "main"
            -Fo ${SPIRV}
            -fvk-use-scalar-layout
            -fspv-target-env=vulkan1.3
    )

    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${HLSL_COMPILE_CMD}
            DEPENDS ${HLSL}
            COMMENT "Compiling HLSL Shader: ${HLSL}"
    )

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(HLSL)

# spirv-opt generates invalid spirv with debug info, so debug info is disabled for now
#set(SLANG_COMPILE_DEBUG_INFO $<IF:$<CONFIG:Debug>,-g,-g0>)
#set(SLANG_COMPILE_OPT $<IF:$<CONFIG:Debug>,-O0,-O2>)

set(SLANG_COMPILE_DEBUG_INFO -g0)
set(SLANG_COMPILE_OPT -O2)

foreach(SLANG ${SLANG_FILES})
    get_filename_component(FILE_NAME ${SLANG} NAME)
    set(SPIRV "${SPIRV_DIR}/${FILE_NAME}.spv")

    set(SLANG_COMPILE_CMD
            ${SLANG_COMPILER} ${SLANG}
            -target spirv
            -profile spirv_1_6
            -force-glsl-scalar-layout
            ${SLANG_COMPILE_OPT}
            ${SLANG_COMPILE_DEBUG_INFO}
            -o ${SPIRV}
            -entry main
    )

    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${SLANG_COMPILE_CMD}
            DEPENDS ${SLANG}
            COMMENT "Compiling SLANG Shader: ${SLANG}"
    )

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(SLANG)

add_custom_target(Shaders ALL DEPENDS ${SPIRV_BINARY_FILES})

add_dependencies(Grace Shaders)
