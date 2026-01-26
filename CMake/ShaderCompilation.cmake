include(${PROJECT_SOURCE_DIR}/CMake/ValidateShaderCompiler.cmake)

if (NOT DEFINED GRACE_CUSTOM_GLSL_COMPILER)
    find_program(GLSL_COMPILER glslc HINTS Vulkan_BIN_DIR)
    if (EXISTS ${GLSL_COMPILER})
        message(STATUS "[GRACE_INFO] Found GLSL to SPIRV compiler (${GLSL_COMPILER})")
    endif ()
else ()
    set(GLSL_COMPILER ${GRACE_CUSTOM_GLSL_COMPILER})
    message(STATUS "[GRACE_INFO] Detected path to custom GLSL compiler (${GLSL_COMPILER})")
    ValidateShaderCompiler(${GLSL_COMPILER})
endif ()

if (NOT DEFINED GRACE_CUSTOM_HLSL_COMPILER)
    find_program(HLSL_COMPILER dxc HINTS Vulkan_BIN_DIR)
    if (EXISTS ${HLSL_COMPILER})
        message(STATUS "[GRACE_INFO] Found HLSL to SPIRV compiler (${HLSL_COMPILER})")
    endif ()
else ()
    set(HLSL_COMPILER ${GRACE_CUSTOM_HLSL_COMPILER})
    message(STATUS "[GRACE_INFO] Detected path to custom HLSL compiler (${HLSL_COMPILER})")
    ValidateShaderCompiler(${HLSL_COMPILER})
endif ()

if (NOT DEFINED GRACE_CUSTOM_SLANG_COMPILER)
    find_program(SLANG_COMPILER slangc HINTS Vulkan_BIN_DIR)
    if (EXISTS ${SLANG_COMPILER})
        message(STATUS "[GRACE_INFO] Found SLANG to SPIRV compiler (${SLANG_COMPILER})")
    endif ()
else ()
    set(SLANG_COMPILER ${GRACE_CUSTOM_SLANG_COMPILER})
    message(STATUS "[GRACE_INFO] Detected path to custom SLANG compiler (${SLANG_COMPILER})")
    ValidateShaderCompiler(${SLANG_COMPILER})
endif ()

if (NOT DEFINED GRACE_GLSL_SHADERS)
    message(STATUS "[GRACE_INFO] GRACE_GLSL_SHADERS not defined. Auto-searching for glsl shaders...")
    file(GLOB_RECURSE GRACE_GLSL_SHADERS
        "${GRACE_SHADERS_DIR}*.frag"
        "${GRACE_SHADERS_DIR}*.vert"
        "${GRACE_SHADERS_DIR}*.comp"
    )
endif ()

if (NOT DEFINED GRACE_HLSL_SHADERS)
    message(STATUS "[GRACE_INFO] GRACE_HLSL_SHADERS not defined. Auto-searching for hlsl shaders...")
    file(GLOB_RECURSE GRACE_HLSL_SHADERS
        "${GRACE_SHADERS_DIR}*.hlsl"
    )
endif ()

if (NOT DEFINED GRACE_SLANG_SHADERS)
    message(STATUS "[GRACE_INFO] GRACE_SLANG_SHADERS not defined. Auto-searching for slang shaders...")
    file(GLOB_RECURSE GRACE_SLANG_SHADERS
        "${GRACE_SHADERS_DIR}*.slang"
    )
endif ()

# Note for PY_SHADER_COMPILATION_CMD:
# The preceding space before the extra compiler args are required to prevent
# them being interpreted as arguments for the script itself. This seems to only
# be a problem with just one extra arg specified (no spaces)

set(PY_SHADER_COMPILATION_CMD
    py ${GRACE_PY_SHADER_COMP_SCRIPT}
    --shaders ${GRACE_GLSL_SHADERS} ${GRACE_SLANG_SHADERS} ${GRACE_HLSL_SHADERS}
    --spirv-dir ${GRACE_SPIRV_DIR}
    --slangc ${SLANG_COMPILER}
    --slangc-args " ${GRACE_SLANG_COMPILER_ARGS}"
    --hlslc ${HLSL_COMPILER}
    --hlslc-args " ${GRACE_HLSL_COMPILER_ARGS}"
    --glslc ${GLSL_COMPILER}
    --glslc-args " ${GRACE_GLSL_COMPILER_ARGS}"
)

add_custom_target(
    ShaderCompilation ALL
    COMMAND ${PY_SHADER_COMPILATION_CMD}
)

add_dependencies(Grace ShaderCompilation)