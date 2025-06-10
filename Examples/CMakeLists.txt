cmake_minimum_required(VERSION 3.15)
project(GraceExamples)

include(FetchContent)

FetchContent_Declare(
        glfw
        EXCLUDE_FROM_ALL
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)

FetchContent_MakeAvailable(glfw)

add_executable(GraceExamples)

target_sources(GraceExamples PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/HelloTriangle/HelloTriangle.cpp)

target_link_libraries(GraceExamples PRIVATE Grace glfw)

target_compile_definitions(GraceExamples PRIVATE USE_GLFW)
