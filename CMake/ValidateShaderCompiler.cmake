set(VALID_SHADER_COMPILERS
    slangc
    glslang
    glslc
    dxc
)

string(REPLACE ";" ", " VALID_SHADER_COMPILERS_PRETTY_PRINT "${VALID_SHADER_COMPILERS}")

function(validate_shader_compiler SHADER_COMPILER_PATH)
    get_filename_component(FILE_NAME ${SHADER_COMPILER_PATH} NAME_WE)
    if (NOT FILE_NAME IN_LIST VALID_SHADER_COMPILERS)
        message(FATAL_ERROR
                "[GRACE_ERROR] Shader compiler '${SHADER_COMPILER_PATH}' is not a valid compiler. "
                "Must be one of: ${VALID_SHADER_COMPILERS_PRETTY_PRINT}!"
        )
    endif ()
endfunction()