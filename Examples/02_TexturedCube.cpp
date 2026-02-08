#include <chrono>
#include <format>

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include <Grace/Grace.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

struct FrameData
{
    Grace::CommandPool* pCmdPool = nullptr;
    Grace::CommandBuffer cmd = {};
    Grace::FenceHandle inFlightFence = {};
};

struct Camera
{
    glm::vec3 position = { 0.0F, 0.0F, 0.0F };
    glm::vec3 rotation = { 0.0F, 0.0F, 0.0F };
};

struct Vertex
{
    glm::vec3 position = { 0.0F, 0.0F, 0.0F };
    glm::vec2 uv = { 0.0F, 0.0F };
};

static glm::mat4 HandleCamera(GLFWwindow* pWindow, Camera& camera, float dt, float speed, float sens);

int main()
{
    constexpr uint32_t FRAMES_IN_FLIGHT = 2;

    // Timings
    double texturedCubePassTime = 0.0;
    double gpuFrameTime = 0.0;
    float cpuFrameTime = 0.0F;

    // Options
    const float cameraSpeed = 10.0F;
    const float cameraSensitivity = 90.0F;
    bool vsync = false;
    bool framebufferHasResized = false;
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;

    glfwInit();
    const GLFWvidmode* vm = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_POSITION_X, (vm->width - windowWidth) / 2);
    glfwWindowHint(GLFW_POSITION_Y, (vm->height - windowHeight) / 2);
    GLFWwindow* pWindow = glfwCreateWindow(windowWidth, windowHeight, "Textured Cube", nullptr, nullptr);
    glfwSetWindowUserPointer(pWindow, &framebufferHasResized);
    glfwSetFramebufferSizeCallback(pWindow, [](GLFWwindow* pWindow, int width, int height) {
        bool& self = *static_cast<bool*>(glfwGetWindowUserPointer(pWindow));
        self = true;
    });

    uint32_t glfwInstanceExtCount = 0;
    const char** glfwInstanceExt = glfwGetRequiredInstanceExtensions(&glfwInstanceExtCount);
    Grace::Context gpuContext({ .extensions = std::vector(glfwInstanceExt, glfwInstanceExt + glfwInstanceExtCount) });

    VkSurfaceKHR surfaceKHR = nullptr;
    glfwCreateWindowSurface(gpuContext.GetInstance(), pWindow, nullptr, &surfaceKHR);
    Grace::Device* pDevice = gpuContext.DevicePtr({
        .maxImageDescriptors = 65535,
        .maxSamplerDescriptors = 65535,
        .maxBufferDescriptors = 65535,
        .framesInFlight = FRAMES_IN_FLIGHT,
        .requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME },
        .queryGroupDesc = {
            .timestampQueriesCount = 8,
            .pipelineStatisticsFlags = Grace::QueryStats::FragmentShaderInvocations,
        },
        .surfacekhr = surfaceKHR,
    });

    // Must first create the swapchain with desired extents
    pDevice->CreateSwapchain({ windowWidth, windowHeight }, vsync);

    std::array<FrameData, FRAMES_IN_FLIGHT> frame = {};
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        const std::string cmdPoolDebugName = "Example02::pCmdPool::" + std::to_string(i);
        Grace::CommandPool* pCmdPool = pDevice->GetCommandPool(Grace::QueueFamily::Graphics, cmdPoolDebugName.c_str());
        frame[i] = {
            .pCmdPool = pCmdPool,
            .cmd = pCmdPool->GetOrAllocateCommandBuffer(),
        };

        const std::string fenceDebugName = "Example02::inFlightFence::" + std::to_string(i);
        frame[i].inFlightFence = pDevice->CreateFence({
            .name = fenceDebugName.c_str(),
            .flags = Grace::FenceFlags::CreateSignalled,
        });
    }

    const Grace::SamplerHandle linearWrapSampler = pDevice->CreateSampler({
        .minFilter = Grace::Filter::Linear,
        .magFilter = Grace::Filter::Linear,
        .addressMode = Grace::SamplerAddressMode::ClampToEdge,
        .mipmapMode = Grace::SamplerMipmapMode::Linear,
    });

    Grace::ImageHandle depthImg = pDevice->CreateImage({
        .name = "Example02::depthImg",
        .dimensions = Grace::UInt3(windowWidth, windowHeight, 1),
        .format = Grace::Format::D32_SFloat,
        .usage = Grace::ImageUsage::DepthStencilAttachment,
        .access = Grace::AccessType::DepthStencilAttachmentReadWrite,
        .size = 0,
        .data = nullptr,
        .mipmapped = false,
    });

    const Grace::PipelineHandle texturedCubePipeline = pDevice->CreateGraphicsPipeline({
        .name = "Example02::texturedCubePipeline",
        .shaders = {
            Grace::ShaderDesc(Grace::ShaderStage::Vertex, "02_TexturedCube.vert.spv"),
            Grace::ShaderDesc(Grace::ShaderStage::Fragment, "02_TexturedCube.frag.spv"),
        },
        .graphicsState = {
            .colourAttachmentFormats = { Grace::Format::RGBA8_SRGB },
            .depthAttachmentFormat = Grace::Format::D32_SFloat,
            .topology = Grace::Topology::TriangleList,
            .polygonMode = Grace::PolygonMode::Fill,
            .cullMode = Grace::CullMode::None,
            .frontFace = Grace::FrontFace::Clockwise,
            .depthStencilUsage = Grace::DepthStencilUsage::DepthOnly,
            .depthCompareOp = Grace::CompareOp::GreaterOrEqual,
        },
        .layout = pDevice->GetSolePipelineLayout(),
    });

    // Cube vertex and index buffer
    // clang-format off
     constexpr std::array<Vertex, 36> vertices = {
        Vertex(glm::vec3(-0.5F, -0.5F, -0.5F),  glm::vec2(0.0F, 0.0F)),
        Vertex(glm::vec3( 0.5F, -0.5F, -0.5F),  glm::vec2(1.0F, 0.0F)),
        Vertex(glm::vec3( 0.5F,  0.5F, -0.5F),  glm::vec2(1.0F, 1.0F)),
        Vertex(glm::vec3( 0.5F,  0.5F, -0.5F),  glm::vec2(1.0F, 1.0F)),
        Vertex(glm::vec3(-0.5F,  0.5F, -0.5F),  glm::vec2(0.0F, 1.0F)),
        Vertex(glm::vec3(-0.5F, -0.5F, -0.5F),  glm::vec2(0.0F, 0.0F)),

        Vertex(glm::vec3(-0.5F, -0.5F,  0.5F),  glm::vec2(0.0F, 0.0F)),
        Vertex(glm::vec3( 0.5F, -0.5F,  0.5F),  glm::vec2(1.0F, 0.0F)),
        Vertex(glm::vec3( 0.5F,  0.5F,  0.5F),  glm::vec2(1.0F, 1.0F)),
        Vertex(glm::vec3( 0.5F,  0.5F,  0.5F),  glm::vec2(1.0F, 1.0F)),
        Vertex(glm::vec3(-0.5F,  0.5F,  0.5F),  glm::vec2(0.0F, 1.0F)),
        Vertex(glm::vec3(-0.5F, -0.5F,  0.5F),  glm::vec2(0.0F, 0.0F)),

        Vertex(glm::vec3(-0.5F,  0.5F,  0.5F),  glm::vec2(1.0F, 0.0F)),
        Vertex(glm::vec3(-0.5F,  0.5F, -0.5F),  glm::vec2(1.0F, 1.0F)),
        Vertex(glm::vec3(-0.5F, -0.5F, -0.5F),  glm::vec2(0.0F, 1.0F)),
        Vertex(glm::vec3(-0.5F, -0.5F, -0.5F),  glm::vec2(0.0F, 1.0F)),
        Vertex(glm::vec3(-0.5F, -0.5F,  0.5F),  glm::vec2(0.0F, 0.0F)),
        Vertex(glm::vec3(-0.5F,  0.5F,  0.5F),  glm::vec2(1.0F, 0.0F)),

        Vertex(glm::vec3(0.5F,  0.5F,  0.5F),  glm::vec2(1.0F, 0.0F)),
        Vertex(glm::vec3(0.5F,  0.5F, -0.5F),  glm::vec2(1.0F, 1.0F)),
        Vertex(glm::vec3(0.5F, -0.5F, -0.5F),  glm::vec2(0.0F, 1.0F)),
        Vertex(glm::vec3(0.5F, -0.5F, -0.5F),  glm::vec2(0.0F, 1.0F)),
        Vertex(glm::vec3(0.5F, -0.5F,  0.5F),  glm::vec2(0.0F, 0.0F)),
        Vertex(glm::vec3(0.5F,  0.5F,  0.5F),  glm::vec2(1.0F, 0.0F)),

        Vertex(glm::vec3(-0.5F, -0.5F, -0.5F), glm::vec2(0.0F, 1.0F)),
        Vertex(glm::vec3( 0.5F, -0.5F, -0.5F), glm::vec2(1.0F, 1.0F)),
        Vertex(glm::vec3( 0.5F, -0.5F,  0.5F), glm::vec2(1.0F, 0.0F)),
        Vertex(glm::vec3( 0.5F, -0.5F,  0.5F), glm::vec2(1.0F, 0.0F)),
        Vertex(glm::vec3(-0.5F, -0.5F,  0.5F), glm::vec2(0.0F, 0.0F)),
        Vertex(glm::vec3(-0.5F, -0.5F, -0.5F), glm::vec2(0.0F, 1.0F)),

        Vertex(glm::vec3(-0.5F,  0.5F, -0.5F),  glm::vec2(0.0F, 1.0F)),
        Vertex(glm::vec3( 0.5F,  0.5F, -0.5F),  glm::vec2(1.0F, 1.0F)),
        Vertex(glm::vec3( 0.5F,  0.5F,  0.5F),  glm::vec2(1.0F, 0.0F)),
        Vertex(glm::vec3( 0.5F,  0.5F,  0.5F),  glm::vec2(1.0F, 0.0F)),
        Vertex(glm::vec3(-0.5F,  0.5F,  0.5F),  glm::vec2(0.0F, 0.0F)),
        Vertex(glm::vec3(-0.5F,  0.5F, -0.5F),  glm::vec2(0.0F, 1.0F))
    };
    // clang-format on

    const Grace::BufferHandle vertexBuffer = pDevice->CreateBuffer({
        .name = "Example02::vertexBuffer",
        .usage =
            Grace::BufferUsage::StorageBuffer | Grace::BufferUsage::DeviceAddress | Grace::BufferUsage::TransferDst,
        .allocFlags = 0,
        .size = vertices.size() * sizeof(Vertex),
        .data = vertices.data(),
    });

    // Load image from file using stbi
    int x, y, channels;
    uint8_t* data = stbi_load(IMAGES_PATH "UVCheckerMap10-1024.png", &x, &y, &channels, 4);
    const uint32_t textureSizeBytes = x * y * 4 * sizeof(uint8_t);

    // Create an image with image file metadata
    const Grace::ImageHandle texture = pDevice->CreateImage({
        .name = "Example02::texture",
        .dimensions = Grace::UInt3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), 1),
        .format = Grace::Format::RGBA8_SRGB,
        .usage = Grace::ImageUsage::SampledImage | Grace::ImageUsage::TransferDst,
        .size = textureSizeBytes,
        .data = data,
        .mipmapped = true,
    });
    stbi_image_free(data);

    // MVP
    Camera cam = {};
    cam.position.z = 3.0F;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -6.0f));
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::perspectiveFov(
        glm::radians(45.0f), static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.001F, 1000.0F);
    proj[1][1] *= -1.0f;

    glm::mat4 mvp = proj * view * model;

    /* Render loop */
    std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(pWindow))
    {
        // Convert units to seconds
        const float dt = cpuFrameTime * 0.001F;
        glfwPollEvents();

        model = glm::rotate(model, 0.5F * dt, glm::vec3(1.0F, 1.0F, 1.0F));
        view = HandleCamera(pWindow, cam, dt, cameraSpeed, cameraSensitivity);
        proj = glm::perspectiveFov(
            glm::radians(45.0f), static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.001F, 1000.0F);
        proj[1][1] *= -1.0f;
        mvp = proj * view * model;

        /* Prepare the frame */

        const uint32_t frameIndex = pDevice->GetCurrentFrameInFlightIndex();

        // The inFlightFences are created with signal bit, so they will already start signalled for the first use
        pDevice->WaitForFence(frame[frameIndex].inFlightFence);

        // Acquire an available image from the swapchain
        const Grace::FrameSyncGroup& fsg = pDevice->AcquireNextSwapchainImage({ windowWidth, windowHeight });

        // Reset the fence only when work has been submitted, otherwise next frame will be waiting on 'work'
        // to finish indefinitely
        pDevice->ResetFence(frame[frameIndex].inFlightFence);

        // Reset command pool, which will reset all command buffers allocated from it too
        frame[frameIndex].pCmdPool->Reset();

        pDevice->UpdateBindlessDescriptorSet();

        // Now that the command buffer has been reset, we can start recording for the subsequent frame
        Grace::CommandBuffer& cmd = frame[frameIndex].cmd;
        cmd.BeginRecording();
        cmd.ResetQueryPoolFullRange<Grace::QueryType::Timestamp>(frameIndex);

        cmd.WriteTimestamp("GPU Frame Begin", Grace::PipelineStage::AllCommands, frameIndex);

        /* Record commands */

        const Grace::ImageHandle& swapchainImg = pDevice->GetRecentlyAcquiredSwapchainImage();

        // Transition swapchain image to a writable layout
        cmd.AddImageBarrier(swapchainImg, { Grace::AccessType::None }, { Grace::AccessType::ClearWrite });
        cmd.PipelineBarrier();

        cmd.ClearColorImage(swapchainImg, Grace::Float4(0.35F, 0.55F, 0.85F, 1.0F));

        cmd.AddImageBarrier(
            swapchainImg, { Grace::AccessType::ClearWrite }, { Grace::AccessType::ColorAttachmentReadWrite });
        cmd.PipelineBarrier();

        cmd.BeginDebugLabel("Textured Cube Pass");
        cmd.BeginDynamicRendering({
            .renderArea = { .extent = { windowWidth, windowHeight } },
            .colorAttachments = { Grace::ColourAttachmentInfo(pDevice->GetImage(swapchainImg), nullptr) },
            .depthAttachments = { Grace::DepthAttachmentInfo(pDevice->GetImage(depthImg)) },
        });

        cmd.SetViewport({ {
            .x = 0,
            .y = 0,
            .width = static_cast<float>(windowWidth),
            .height = static_cast<float>(windowHeight),
            .minDepth = 1.0F, // Reverse depth for better near plane precision
            .maxDepth = 0.0F,
        } });

        cmd.SetScissor({ {
            .offset = { 0, 0 },
            .extent = { windowWidth, windowHeight },
        } });

        // Bind helloTriangle pipeline and execute a draw call
        cmd.BindPipeline(texturedCubePipeline);
        cmd.BindDescriptorSets(
            Grace::PipelineBindPoint::Graphics, pDevice->GetSolePipelineLayout(), { pDevice->GetSoleDescriptorSet() });

        struct PC
        {
            uint64_t vbuffer;
            glm::mat4 mvp;
            uint32_t textureIndex;
            uint32_t linearWrapSamplerIndex;
        } pc;

        pc.vbuffer = pDevice->GetBuffer(vertexBuffer).GetBDA();
        pc.mvp = mvp;
        pc.textureIndex = pDevice->GetImage(texture).GetSampledImgId();
        pc.linearWrapSamplerIndex = pDevice->GetSampler(linearWrapSampler).GetSamplerId();
        cmd.PushConstants(pDevice->GetSolePipelineLayout(), &pc);

        cmd.WriteTimestamp("TexturedCube Pass Begin", Grace::PipelineStage::VertexShader, frameIndex);
        cmd.Draw(vertices.size(), 1, 0, 0);
        cmd.WriteTimestamp("TexturedCube Pass End", Grace::PipelineStage::FragmentShader, frameIndex);

        cmd.EndDynamicRendering();
        cmd.EndDebugLabel();

        // Transition swapchain image to presentable layout
        cmd.AddImageBarrier(pDevice->GetRecentlyAcquiredSwapchainImage(),
                            { Grace::AccessType::ColorAttachmentReadWrite },
                            { Grace::AccessType::Present });
        cmd.PipelineBarrier();

        /* Wrap up the frame */

        cmd.WriteTimestamp("GPU Frame End", Grace::PipelineStage::AllCommands, frameIndex);

        // Finish recording for the command buffer for this frame
        cmd.EndRecording();

        // Submit the command buffer
        // When using the swapchain, the device needs to be certain that, at this point,
        // there is a swapchain image available since it will be writing to it - it will
        // wait on the acquireSemaphore to be signalled
        // Once the submission is complete, one can be sure that the swapchain image is no
        // longer being written to, so presentSemaphore is signalled
        pDevice->Submit(Grace::QueueFamily::Graphics, cmd, fsg, frame[frameIndex].inFlightFence);

        // Present image as soon as it is safe to do so - when presentSemaphore is signalled
        const Grace::SwapchainStatus ss = pDevice->Present(fsg);

        const Grace::TimestampQueryGroup& tqg =
            pDevice->GetQueryPoolResults<Grace::QueryType::Timestamp>(0, 0, Grace::QueryResult::WithAvailability);

        tqg.DurationIfAvailable<Grace::TimestampUnits::Milliseconds>(
            texturedCubePassTime, "TexturedCube Pass Begin", "TexturedCube Pass End", frameIndex);

        tqg.DurationIfAvailable<Grace::TimestampUnits::Milliseconds>(
            gpuFrameTime, "GPU Frame Begin", "GPU Frame End", frameIndex);

        if (ss == Grace::SwapchainStatus::ShouldResize || framebufferHasResized)
        {
            framebufferHasResized = false;

            // Handle minimisation
            int width = 0;
            int height = 0;
            glfwGetFramebufferSize(pWindow, &width, &height);
            while (width == 0 || height == 0)
            {
                glfwGetFramebufferSize(pWindow, &width, &height);
                glfwWaitEvents();
            }
            windowWidth = width;
            windowHeight = height;

            pDevice->CreateSwapchain({ windowWidth, windowHeight }, vsync);
            pDevice->FreeImage(depthImg);
            depthImg = pDevice->CreateImage({
                .name = "Example02::depthImg",
                .dimensions = Grace::UInt3(windowWidth, windowHeight, 1),
                .format = Grace::Format::D32_SFloat,
                .usage = Grace::ImageUsage::DepthStencilAttachment,
                .access = Grace::AccessType::DepthStencilAttachmentReadWrite,
                .size = 0,
                .data = nullptr,
                .mipmapped = false,
            });
        } else if (ss == Grace::SwapchainStatus::Failure)
        {
            break;
        }

        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        cpuFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count() / 1000.0F;
        lastTime = now;

        std::string windowTitle = std::format("CPU Frame Time: {}ms | GPU Frame Time: {}ms | TexturedCube Pass: {}ms",
                                              cpuFrameTime,
                                              static_cast<float>(gpuFrameTime),
                                              static_cast<float>(texturedCubePassTime));
        glfwSetWindowTitle(pWindow, windowTitle.c_str());

        pDevice->AdvanceToNextFrame();
    }

    glfwTerminate();
    return 0;
}

glm::mat4 HandleCamera(GLFWwindow* pWindow, Camera& camera, float dt, float speed, float sens)
{
    glm::vec3 rot = glm::vec3(0.0F);
    if (glfwGetKey(pWindow, GLFW_KEY_UP) == GLFW_PRESS)
    {
        rot.x += 1.0F;
    }
    if (glfwGetKey(pWindow, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        rot.x -= 1.0F;
    }
    if (glfwGetKey(pWindow, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        rot.y += 1.0F;
    }
    if (glfwGetKey(pWindow, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        rot.y -= 1.0F;
    }
    if (glm::any(glm::notEqual(rot, glm::vec3(0.0F))))
    {
        camera.rotation += glm::normalize(rot) * sens * dt;
    }

    camera.rotation.y = glm::mod(camera.rotation.y, 360.0F);
    camera.rotation.x = glm::clamp(camera.rotation.x, -89.0F, 89.0F);

    glm::quat pitchRotation = glm::angleAxis(glm::radians(camera.rotation.x), glm::vec3 { 1.0F, 0.0F, 0.0F });
    glm::quat yawRotation = glm::angleAxis(glm::radians(camera.rotation.y), glm::vec3 { 0.0F, 1.0F, 0.0F });

    glm::mat4 R = glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);

    glm::vec3 move = glm::vec3(0.0F);
    if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS)
    {
        move.z -= 1.0F;
    }
    if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS)
    {
        move.z += 1.0F;
    }
    if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS)
    {
        move.x -= 1.0F;
    }
    if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS)
    {
        move.x += 1.0F;
    }
    if (glfwGetKey(pWindow, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        move.y += 1.0F;
    }
    if (glfwGetKey(pWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        move.y -= 1.0F;
    }
    if (glm::any(glm::notEqual(move, glm::vec3(0.0F))))
    {
        camera.position += glm::normalize(glm::mat3(R) * move) * speed * dt;
    }
    glm::mat4 T = glm::translate(glm::mat4(1.0F), camera.position);

    return glm::inverse(T * R);
}
