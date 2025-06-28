cmake_minimum_required(VERSION 3.25)

project(GraceLib)

# Set these in the top level CMakeLists.txt
set(GRACE_SPIRV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Resources/SPIRV/)
set(GRACE_SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Resources/Shaders/)
set(GRACE_DESIRED_VULKAN_SDK_VERSION "1.4.313.0")

add_library(Grace SHARED)

add_subdirectory(Source)

option(GRACE_COMPILE_EXAMPLES "Compile examples" ON)

if(GRACE_COMPILE_EXAMPLES)
    add_subdirectory(Examples)
endif()
