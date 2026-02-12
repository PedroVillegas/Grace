include(FetchContent)

FetchContent_Declare(
    glfw
    EXCLUDE_FROM_ALL
    DOWNLOAD_EXTRACT_TIMESTAMP true
    URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG f0569113c93ad095470c54bf34a17b36646bbbb5 # Release - ???
)

FetchContent_Declare(
    glm
    DOWNLOAD_EXTRACT_TIMESTAMP true
    URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
)

FetchContent_MakeAvailable(glfw stb glm)

add_executable(HelloTriangle)
target_sources(HelloTriangle PRIVATE 01_HelloTriangle/01_HelloTriangle.cpp)
target_link_libraries(HelloTriangle PRIVATE Grace glfw)
target_compile_definitions(HelloTriangle PRIVATE CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/01_HelloTriangle/01_GraceConfig.yaml")

add_executable(TexturedCube)
target_sources(TexturedCube PRIVATE 02_TexturedCube/02_TexturedCube.cpp)
target_link_libraries(TexturedCube PRIVATE Grace glfw glm)
target_include_directories(TexturedCube PRIVATE ${stb_SOURCE_DIR})
target_compile_definitions(TexturedCube
	PRIVATE IMAGES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/Assets/Images/"
	PRIVATE CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/02_TexturedCube/02_GraceConfig.yaml"
)

add_executable(ComputeShader)
target_sources(ComputeShader PRIVATE 03_ComputeShader/03_ComputeShader.cpp)
target_link_libraries(ComputeShader PRIVATE Grace glfw)
target_compile_definitions(ComputeShader PRIVATE CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/03_ComputeShader/03_GraceConfig.yaml")

add_executable(StreamCompaction)
target_sources(StreamCompaction PRIVATE 04_StreamCompaction/04_StreamCompaction.cpp)
target_link_libraries(StreamCompaction PRIVATE Grace)
target_compile_definitions(StreamCompaction PRIVATE CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/04_StreamCompaction/04_GraceConfig.yaml")
