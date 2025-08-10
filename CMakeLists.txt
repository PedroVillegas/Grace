cmake_minimum_required(VERSION 3.25)

project(GraceLib)

option(GRACE_COMPILE_EXAMPLES "Compile examples" ON)

if(GRACE_COMPILE_EXAMPLES)
    # Define these in the CMakeLists.txt where Grace is included
    set(GRACE_SPIRV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Resources/SPIRV/)
    set(GRACE_SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Resources/Shaders/)
    set(GRACE_TARGET_VULKAN_API_VERSION 13)
    set(GRACE_DESIRED_VULKAN_SDK_VERSION "1.3.296.0")

    # Define these to use custom shader compilers (default compilers used are searched for in Vulkan SDK directory
    # set(GRACE_CUSTOM_GLSL_COMPILER ${PATH_TO_GLSL_COMPILER})
    # set(GRACE_CUSTOM_HLSL_COMPILER ${PATH_TO_HLSL_COMPILER})
    set(GRACE_CUSTOM_SLANG_COMPILER ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Bin/slang-2025.13/bin/slangc.exe)
endif()

add_library(Grace SHARED)

add_subdirectory(Source)

if(GRACE_COMPILE_EXAMPLES)
    add_subdirectory(Examples)
endif()
