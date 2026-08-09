cmake_minimum_required(VERSION 3.25)

project(Grace VERSION 1.0.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(CMakePrintHelpers)

option(GRACE_ENABLE_RUNTIME_SHADER_COMP_EXT "Enables runtime shader compilation functionality" OFF)
option(GRACE_ENABLE_IMGUI_EXT "Enables custom imgui renderer backend" OFF)

if(${CMAKE_PROJECT_NAME} STREQUAL Grace)
    set(GRACE_ENABLE_RUNTIME_SHADER_COMP_EXT ON)
    set(GRACE_ENABLE_IMGUI_EXT ON)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_subdirectory(Source)

if(${CMAKE_PROJECT_NAME} STREQUAL Grace)
    message(STATUS "[GRACE_INFO] Compiling examples")
    add_subdirectory(Examples)

    message(STATUS "[GRACE_INFO] Compiling tests")
    add_subdirectory(Tests)
endif()
