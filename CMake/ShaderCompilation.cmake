if (NOT DEFINED GRACE_CUSTOM_GLSL_COMPILER)
    find_program(GLSL_COMPILER glslc HINTS Vulkan_BIN_DIR)
    if (EXISTS ${GLSL_COMPILER})
        message(STATUS "[GRACE_INFO] Found GLSL to SPIRV compiler (${GLSL_COMPILER})")
    endif()
else()
    set(GLSL_COMPILER ${GRACE_CUSTOM_GLSL_COMPILER})
    message(STATUS "[GRACE_INFO] Detected path to custom GLSL compiler (${GLSL_COMPILER})")
endif()

if (NOT DEFINED GRACE_CUSTOM_HLSL_COMPILER)
    find_program(HLSL_COMPILER dxc HINTS Vulkan_BIN_DIR)
    if (EXISTS ${HLSL_COMPILER})
        message(STATUS "[GRACE_INFO] Found HLSL to SPIRV compiler (${HLSL_COMPILER})")
    endif()
else()
    set(HLSL_COMPILER ${GRACE_CUSTOM_HLSL_COMPILER})
    message(STATUS "[GRACE_INFO] Detected path to custom HLSL compiler (${HLSL_COMPILER})")
endif()

if (NOT DEFINED GRACE_CUSTOM_SLANG_COMPILER)
    find_program(SLANG_COMPILER slangc HINTS Vulkan_BIN_DIR)
    if (EXISTS ${SLANG_COMPILER})
        message(STATUS "[GRACE_INFO] Found SLANG to SPIRV compiler (${SLANG_COMPILER})")
    endif()
else()
    set(SLANG_COMPILER ${GRACE_CUSTOM_SLANG_COMPILER})
    message(STATUS "[GRACE_INFO] Detected path to custom SLANG compiler (${SLANG_COMPILER})")
endif()

if (NOT DEFINED GRACE_GLSL_SHADERS)
    message(STATUS "[GRACE_INFO] GRACE_GLSL_SHADERS not defined. Auto-searching for glsl shaders...")
    file(GLOB_RECURSE GRACE_GLSL_SHADERS
        "${GRACE_SHADERS_DIR}/*.frag"
        "${GRACE_SHADERS_DIR}/*.vert"
        "${GRACE_SHADERS_DIR}/*.comp"
    )
endif()

if (NOT DEFINED GRACE_HLSL_SHADERS)
    message(STATUS "[GRACE_INFO] GRACE_HLSL_SHADERS not defined. Auto-searching for hlsl shaders...")
    file(GLOB_RECURSE GRACE_HLSL_SHADERS
        "${GRACE_SHADERS_DIR}/*.hlsl"
    )
endif()

if (NOT DEFINED GRACE_SLANG_SHADERS)
    message(STATUS "[GRACE_INFO] GRACE_SLANG_SHADERS not defined. Auto-searching for slang shaders...")
    file(GLOB_RECURSE GRACE_SLANG_SHADERS
        "${GRACE_SHADERS_DIR}/*.slang"
    )
endif()

set(GLSL_COMPILER_ARGS "")
if (DEFINED GRACE_GLSL_COMPILER_ARGS)
    set(GLSL_COMPILER_ARGS ${GRACE_GLSL_COMPILER_ARGS})
endif()

foreach(GLSL ${GRACE_GLSL_SHADERS})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${GRACE_SPIRV_DIR}/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_COMPILER} ${GLSL} -g -o ${SPIRV} --target-env=vulkan1.3 ${GLSL_COMPILER_ARGS}
        DEPENDS ${GLSL}
        COMMENT "Compiling GLSL Shader: ${GLSL}"
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

set(HLSL_COMPILER_ARGS "")
if (DEFINED GRACE_HLSL_COMPILER_ARGS)
    set(HLSL_COMPILER_ARGS ${GRACE_HLSL_COMPILER_ARGS})
endif()

foreach(HLSL ${GRACE_HLSL_SHADERS})
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
    set(SPIRV "${GRACE_SPIRV_DIR}/${FILE_NAME}.spv")

    set(HLSL_COMPILE_CMD
        ${HLSL_COMPILER} ${HLSL}
        -spirv
        -HV 2021
        -T ${TARGET_PROFILE}
        -E "main"
        -Fo ${SPIRV}
        -fspv-target-env=vulkan1.3
        ${HLSL_COMPILER_ARGS}
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
#set(SLANG_COMPILER_DEBUG_INFO $<IF:$<CONFIG:Debug>,-g,-g0>)
#set(SLANG_COMPILER_OPT $<IF:$<CONFIG:Debug>,-O0,-O2>)

set(SLANG_COMPILER_DEBUG_INFO -g0)
set(SLANG_COMPILER_OPT -O2)

set(SLANG_COMPILER_ARGS "")
if (DEFINED GRACE_SLANG_COMPILER_ARGS)
    set(SLANG_COMPILER_ARGS ${GRACE_SLANG_COMPILER_ARGS})
endif()

foreach(SLANG ${GRACE_SLANG_SHADERS})
    file(STRINGS ${SLANG} PRAGMA LIMIT_INPUT 23 REGEX "#pragma")
    if ("${PRAGMA}" STREQUAL "#pragma DO_NOT_COMPILE")
        message(STATUS "[GRACE_INFO] Ignoring ${SLANG}")
        continue()
    endif()

    get_filename_component(FILE_NAME ${SLANG} NAME)
    set(SPIRV "${GRACE_SPIRV_DIR}/${FILE_NAME}.spv")

    set(SLANG_COMPILE_CMD
        ${SLANG_COMPILER} ${SLANG}
        -target spirv
        -profile spirv_1_6
        ${SLANG_COMPILER_DEBUG_INFO}
        ${SLANG_COMPILER_OPT}
        -o ${SPIRV}
        -entry main
        ${SLANG_COMPILER_ARGS}
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