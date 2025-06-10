cmake_minimum_required (VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Grace VERSION 1.0)

set(SPIRV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Resources/SPIRV/)
set(SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Resources/Shaders/)

# Add main Grace lib
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS True)
add_library(Grace SHARED)
add_subdirectory(Source)

# Optionally add example program
option(GRACE_COMPILE_EXAMPLES "Compile Examples" ON)

if(GRACE_COMPILE_EXAMPLES)
    add_subdirectory(Examples)
endif()
