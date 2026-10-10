set(GRACE_INTERNAL_VALID_SHADER_COMPILERS
    slangc;glslang;glslc;dxc
    CACHE INTERNAL
    "List of valid shader compilers"
)

function(validate_shader_compiler SHADER_COMPILER_PATH)
    get_filename_component(FILE_NAME ${SHADER_COMPILER_PATH} NAME_WE)
    string(REPLACE ";" ", " VALID_SHADER_COMPILERS_PRETTY_PRINT "${GRACE_INTERNAL_VALID_SHADER_COMPILERS}")
    if (NOT FILE_NAME IN_LIST GRACE_INTERNAL_VALID_SHADER_COMPILERS)
        message(FATAL_ERROR
            "[GRACE_ERROR] Shader compiler '${SHADER_COMPILER_PATH}' is not a valid compiler. "
            "Must be one of: ${VALID_SHADER_COMPILERS_PRETTY_PRINT}!"
        )
    endif ()
endfunction()