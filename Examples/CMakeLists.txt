cmake_minimum_required(VERSION 3.25)
project(GraceExamples)

include(FetchContent)

FetchContent_Declare(
        glfw
        EXCLUDE_FROM_ALL
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)

FetchContent_MakeAvailable(glfw)

add_executable(HelloTriangle)
target_sources(HelloTriangle PRIVATE 01_HelloTriangle.cpp)
target_link_libraries(HelloTriangle PRIVATE Grace glfw)
target_compile_definitions(HelloTriangle PRIVATE USE_GLFW)

add_executable(VkCube)
target_sources(VkCube PRIVATE 02_VkCube.cpp)
target_link_libraries(VkCube PRIVATE Grace glfw)
target_compile_definitions(VkCube PRIVATE USE_GLFW)
