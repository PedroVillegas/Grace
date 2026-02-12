cmake_minimum_required(VERSION 3.25)

project(Grace VERSION 1.0.0)

option(GRACE_COMPILE_EXAMPLES "Compile examples" ON)
option(GRACE_COMPILE_TESTS "Compile tests" ON)
option(GRACE_ENABLE_RUNTIME_SHADER_COMP_EXT "Enables runtime shader compilation functionality" ON)

if(GRACE_COMPILE_EXAMPLES)
    # Define these in the CMakeLists.txt where Grace is included
    set(GRACE_SPIRV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Assets/SPIRV)
    set(GRACE_SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Assets/Shaders)

    # Define these if you don't want to auto recurse and compile shaders (these take precedent)
    # set(GRACE_SLANG_SHADERS shader1.slang shader2.slang)
    # set(GRACE_HLSL_SHADERS shader1.hlsl shader2.hlsl)
    # set(GRACE_GLSL_SHADERS shader1.glsl shader2.glsl)

    # Define these to use custom shader compilers (default compilers used are searched for in Vulkan SDK directory)
    set(GRACE_CUSTOM_SLANG_COMPILER ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Bin/slang-2025.13/bin/slangc.exe)
    # set(GRACE_CUSTOM_HLSL_COMPILER ${PATH_TO_HLSL_COMPILER})
    # set(GRACE_CUSTOM_GLSL_COMPILER ${PATH_TO_GLSL_COMPILER})

    # Define these to specify compiler arguments
    set(GRACE_SLANG_COMPILER_ARGS "-force-glsl-scalar-layout")
    # set(GRACE_HLSL_COMPILER_ARGS "-fvk-use-scalar-layout -example-compiler-arg")
    # set(GRACE_GLSL_COMPILER_ARGS "-example-compiler-arg1 -example-compiler-arg2")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_subdirectory(Source)

if(GRACE_COMPILE_EXAMPLES)
    message(STATUS "[GRACE_INFO] Compiling examples")
    add_subdirectory(Examples)
endif()

if(GRACE_COMPILE_TESTS)
    message(STATUS "[GRACE_INFO] Compiling tests")
    add_subdirectory(Tests)
endif()
