include(ValidateShaderCompiler)

function(grace_target_shader_properties TARGET)
	cmake_parse_arguments(PARSE_ARGV 1 GRACE_PREFIX
		""
		"BASE_DIR;SPV_OUT_DIR"
		""
	)

	if (GRACE_PREFIX_BASE_DIR)
		string(LENGTH ${GRACE_PREFIX_BASE_DIR} DL)
		math(EXPR LAST "${DL}-1" OUTPUT_FORMAT DECIMAL)
		string(SUBSTRING ${GRACE_PREFIX_BASE_DIR} ${LAST} -1 END_CHAR)
		if (END_CHAR MATCHES "/")
			set(PREPEND_DIRECTORY ${GRACE_PREFIX_BASE_DIR})
		else()
			set(PREPEND_DIRECTORY ${GRACE_PREFIX_BASE_DIR}/)
		endif()
	endif()

	set(${TARGET}_GRACE_SHADER_PROP_BASE_DIRECTORY ${PREPEND_DIRECTORY} PARENT_SCOPE)
	set(${TARGET}_GRACE_SHADER_PROP_SPV_OUT_DIRECTORY ${GRACE_PREFIX_SPV_OUT_DIR} PARENT_SCOPE)

	target_compile_definitions(${TARGET} PRIVATE
		${TARGET}_GRACE_SHADER_PROP_BASE_DIRECTORY="${PREPEND_DIRECTORY}"
		${TARGET}_GRACE_SHADER_PROP_SPV_OUT_DIRECTORY="${GRACE_PREFIX_SPV_OUT_DIR}/"
	)
endfunction()

function(grace_target_compile_slang_shaders TARGET)
	cmake_parse_arguments(PARSE_ARGV 1 GRACE_PREFIX
		""
		"COMPILER"
		"SOURCES;INCLUDE_DIRS;COMPILER_ARGS"
	)

	if (NOT DEFINED ${TARGET}_GRACE_SHADER_PROP_BASE_DIRECTORY)
		message(
			"Grace shader property base directory is undefined.\n"
			"Did you forget to call 'grace_target_shader_properties(${TARGET})' "
			"before 'grace_target_compile_slang_shaders(${TARGET})'?"
		)
	endif ()

	foreach (shader ${GRACE_PREFIX_SOURCES})
		list(APPEND ALL_SHADERS ${${TARGET}_GRACE_SHADER_PROP_BASE_DIRECTORY}${shader})
	endforeach ()

	if (NOT DEFINED GRACE_PREFIX_COMPILER)
		find_program(GRACE_PREFIX_COMPILER slangc HINTS Vulkan_BIN_DIR)
		if (EXISTS ${GRACE_PREFIX_COMPILER})
			message(STATUS "[GRACE_INFO] Found SLANG to SPIRV compiler (${GRACE_PREFIX_COMPILER})")
		endif ()
	else ()
		set(GRACE_PREFIX_COMPILER ${GRACE_PREFIX_COMPILER})
		message(STATUS "[GRACE_INFO] Detected path to custom SLANG compiler (${GRACE_PREFIX_COMPILER})")
		validate_shader_compiler(${GRACE_PREFIX_COMPILER})
	endif ()

	list(APPEND GRACE_PREFIX_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/Source)
	list(APPEND GRACE_PREFIX_COMPILER_ARGS -I ${GRACE_PREFIX_INCLUDE_DIRS})
	list(JOIN GRACE_PREFIX_COMPILER_ARGS " ~ " SLANGC_ARGS)

	set(PY_SHADER_COMPILATION_CMD
		py ${PROJECT_SOURCE_DIR}/Scripts/ShaderCompiler.py
		--shaders ${ALL_SHADERS}
		--spirv-dir ${${TARGET}_GRACE_SHADER_PROP_SPV_OUT_DIRECTORY}
		--slangc ${GRACE_PREFIX_COMPILER}
		--slangc-args ${SLANGC_ARGS}
	)

	add_custom_target(
		${TARGET}-ShaderCompilation ALL
		COMMAND ${PY_SHADER_COMPILATION_CMD}
	)

	add_dependencies(${TARGET} ${TARGET}-ShaderCompilation)

	if (NOT DEFINED ${TARGET}_GRACE_SHADER_PROP_SLANG_COMPILER)
		target_compile_definitions(${TARGET} PRIVATE
			${TARGET}_GRACE_SHADER_PROP_SLANG_COMPILER="${GRACE_PREFIX_COMPILER}"
		)
	else ()
		message(
			"Warning: ${TARGET}_GRACE_SHADER_PROP_SLANG_COMPILER is already defined as '${${TARGET}_GRACE_SHADER_PROP_SLANG_COMPILER}'. "
			"Redefining this may break runtime compilation extension."
		)
	endif ()
endfunction()

function(grace_target_compile_glsl_shaders TARGET)
	cmake_parse_arguments(PARSE_ARGV 1 GRACE_PREFIX
		""
		"COMPILER"
		"SOURCES;INCLUDE_DIRS;COMPILER_ARGS"
	)

	if (NOT DEFINED ${TARGET}_GRACE_SHADER_PROP_BASE_DIRECTORY)
		message(
			"Grace shader property base directory is undefined.\n"
			"Did you forget to call 'grace_target_shader_properties(${TARGET})' "
			"before 'grace_target_compile_slang_shaders(${TARGET})'?"
		)
	endif ()

	foreach (shader ${GRACE_PREFIX_SOURCES})
		list(APPEND ALL_SHADERS ${${TARGET}_GRACE_SHADER_PROP_BASE_DIRECTORY}${shader})
	endforeach ()

	if (NOT DEFINED GRACE_PREFIX_COMPILER)
		find_program(GRACE_PREFIX_COMPILER glslc HINTS Vulkan_BIN_DIR)
		if (EXISTS ${GRACE_PREFIX_COMPILER})
			message(STATUS "[GRACE_INFO] Found GLSL to SPIRV compiler (${GRACE_PREFIX_COMPILER})")
		endif ()
	else ()
		set(GRACE_PREFIX_COMPILER ${GRACE_PREFIX_COMPILER})
		message(STATUS "[GRACE_INFO] Detected path to custom GLSL compiler (${GRACE_PREFIX_COMPILER})")
		validate_shader_compiler(${GRACE_PREFIX_COMPILER})
	endif ()

	list(APPEND GRACE_PREFIX_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/Source)
	list(APPEND GRACE_PREFIX_COMPILER_ARGS -I ${GRACE_PREFIX_INCLUDE_DIRS})
	list(JOIN GRACE_PREFIX_COMPILER_ARGS " ~ " GLSLC_ARGS)

	set(PY_SHADER_COMPILATION_CMD
		py ${PROJECT_SOURCE_DIR}/Scripts/ShaderCompiler.py
		--shaders ${ALL_SHADERS}
		--spirv-dir ${${TARGET}_GRACE_SHADER_PROP_SPV_OUT_DIRECTORY}
		--glslc ${GRACE_PREFIX_COMPILER}
		--glslc-args ${GLSLC_ARGS}
	)

	add_custom_target(
		${TARGET}-ShaderCompilation ALL
		COMMAND ${PY_SHADER_COMPILATION_CMD}
	)

	add_dependencies(${TARGET} ${TARGET}-ShaderCompilation)
endfunction()

function(grace_target_compile_hlsl_shaders TARGET)
	cmake_parse_arguments(PARSE_ARGV 1 GRACE_PREFIX
		""
		"BASE_DIR;SPV_OUT_DIR;COMPILER"
		"SOURCES;INCLUDE_DIRS;COMPILER_ARGS"
	)

	if (NOT DEFINED ${TARGET}_GRACE_SHADER_PROP_BASE_DIRECTORY)
		message(
			"Grace shader property base directory is undefined.\n"
			"Did you forget to call 'grace_target_shader_properties(${TARGET})' "
			"before 'grace_target_compile_slang_shaders(${TARGET})'?"
		)
	endif ()

	foreach (shader ${GRACE_PREFIX_SOURCES})
		list(APPEND ALL_SHADERS ${${TARGET}_GRACE_SHADER_PROP_BASE_DIRECTORY}${shader})
	endforeach ()

	if (NOT DEFINED GRACE_PREFIX_COMPILER)
		find_program(GRACE_PREFIX_COMPILER dxc HINTS Vulkan_BIN_DIR)
		if (EXISTS ${GRACE_PREFIX_COMPILER})
			message(STATUS "[GRACE_INFO] Found HLSL to SPIRV compiler (${GRACE_PREFIX_COMPILER})")
		endif ()
	else ()
		set(GRACE_PREFIX_COMPILER ${GRACE_PREFIX_COMPILER})
		message(STATUS "[GRACE_INFO] Detected path to custom HLSL compiler (${GRACE_PREFIX_COMPILER})")
		validate_shader_compiler(${GRACE_PREFIX_COMPILER})
	endif ()

	list(APPEND GRACE_PREFIX_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/Source)
	list(APPEND GRACE_PREFIX_COMPILER_ARGS -I ${GRACE_PREFIX_INCLUDE_DIRS})
	list(JOIN GRACE_PREFIX_COMPILER_ARGS " ~ " HLSLC_ARGS)

	set(PY_SHADER_COMPILATION_CMD
		py ${PROJECT_SOURCE_DIR}/Scripts/ShaderCompiler.py
		--shaders ${ALL_SHADERS}
		--spirv-dir ${${TARGET}_GRACE_SHADER_PROP_SPV_OUT_DIRECTORY}
		--hlslc ${GRACE_PREFIX_COMPILER}
		--hlslc-args ${HLSLC_ARGS}
	)

	add_custom_target(
		${TARGET}-ShaderCompilation ALL
		COMMAND ${PY_SHADER_COMPILATION_CMD}
	)

	add_dependencies(${TARGET} ${TARGET}-ShaderCompilation)
endfunction()
