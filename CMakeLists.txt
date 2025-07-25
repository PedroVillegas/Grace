cmake_minimum_required(VERSION 3.25)

project(GraceLib)

# Define these in the CMakeLists.txt where Grace is included
set(GRACE_SPIRV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Resources/SPIRV/)
set(GRACE_SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Resources/Shaders/)
set(GRACE_DESIRED_VULKAN_SDK_VERSION "1.4.313.0")

# Define these to use custom shader compilers (default compilers used are searched for in Vulkan SDK directory
# set(GRACE_CUSTOM_GLSL_COMPILER ${PATH_TO_GLSL_COMPILER})
# set(GRACE_CUSTOM_HLSL_COMPILER ${PATH_TO_HLSL_COMPILER})
# set(GRACE_CUSTOM_SLANG_COMPILER ${PATH_TO_SLANG_COMPILER})

add_library(Grace SHARED)

add_subdirectory(Source)

option(GRACE_COMPILE_EXAMPLES "Compile examples" ON)

if(GRACE_COMPILE_EXAMPLES)
    add_subdirectory(Examples)
endif()
