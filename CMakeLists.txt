cmake_minimum_required(VERSION 3.25)

project(Grace VERSION 1.0.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(Grace-Shaders)
include(CMakePrintHelpers)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_subdirectory(Source)

if(${CMAKE_PROJECT_NAME} STREQUAL Grace)
    message(STATUS "[GRACE_INFO] Compiling examples")
    add_subdirectory(Examples)

    message(STATUS "[GRACE_INFO] Compiling tests")
    add_subdirectory(Tests)
endif()
