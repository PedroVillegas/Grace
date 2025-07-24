#include <chrono>
#include <format>

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

glm::mat4 HandleCamera(GLFWwindow* pWindow, Camera& camera, float dt, float speed, float sens);

int main()
{
    constexpr uint32_t FRAMES_IN_FLIGHT = 2;
    uint32_t frameIndex = 0;

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
    glfwSetFramebufferSizeCallback(pWindow,
                                   [](GLFWwindow* pWindow, int width, int height)
                                   {
                                       bool& self = *static_cast<bool*>(glfwGetWindowUserPointer(pWindow));
                                       self = true;
                                   });

    const Grace::DeviceDesc deviceDesc = {
        .maxImageDescriptors = 65535,
        .maxSamplerDescriptors = 65535,
        .maxBufferDescriptors = 65535,
        .framesInFlight = FRAMES_IN_FLIGHT,
        .queryGroupDesc = {
            .timestampQueriesCount = 8,
            .pipelineStatisticsFlags = VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT,
        },
        .pGlfwWindow = pWindow,
    };

    Grace::Context gpuContext({ .deviceConfig = deviceDesc });
    Grace::Device* pDevice = gpuContext.GetDevicePtr();

    // Must first create the swapchain with desired extents
    pDevice->CreateSwapchain({ windowWidth, windowHeight }, vsync);

    std::array<FrameData, FRAMES_IN_FLIGHT> frame = {};
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        const std::string cmdPoolDebugName = "CommandPool::" + std::to_string(i);
        Grace::CommandPool* pCmdPool = pDevice->GetCommandPool(Grace::QueueFamily::Graphics, cmdPoolDebugName.c_str());
        frame[i] = {
            .pCmdPool = pCmdPool,
            .cmd = pCmdPool->GetOrAllocateCommandBuffer(),
        };

        const std::string fenceDebugName = "InFlightFence::" + std::to_string(i);
        frame[i].inFlightFence = pDevice->CreateFence({
            .name = fenceDebugName.c_str(),
            .createFlags = VK_FENCE_CREATE_SIGNALED_BIT,
        });
    }

    const Grace::SamplerHandle linearWrapSampler = pDevice->CreateSampler({
        .minFilter = VK_FILTER_LINEAR,
        .magFilter = VK_FILTER_LINEAR,
        .addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    });

    Grace::ImageHandle depthImg = pDevice->CreateImage({
        .name = "Depth Image",
        .dimensions = { windowWidth, windowHeight, 1 },
        .format = VK_FORMAT_D32_SFLOAT,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .mipmapped = false,
    });

    // Create pipeline from hello triangle shader
    Grace::PipelineBuilder pbuilder(pDevice);
    pbuilder.AddShader("02_TexturedCube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    pbuilder.AddShader("02_TexturedCube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    pbuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pbuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    pbuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pbuilder.SetMultisamplingNone();
    pbuilder.DisableBlending();
    pbuilder.SetColourAttachmentFormat(&pDevice->GetSwapchainFormat());
    pbuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pbuilder.SetDepthFormat(VK_FORMAT_D32_SFLOAT);
    pbuilder.BuildGraphicsPipeline("TexturedCube Pipeline", pDevice->GetSolePipelineLayout());

    const Grace::PipelineHandle texturedCubePH = pDevice->CreatePipeline(pbuilder.pipelineDesc);

    // Load image from file using stbi
    int x, y, channels;
    uint8_t* data = stbi_load(IMAGES_PATH "file.png", &x, &y, &channels, 4);
    const uint32_t textureSizeBytes = x * y * channels * sizeof(uint8_t);

    // Create an image with image file metadata
    const Grace::ImageHandle texture = pDevice->CreateImage({
        .name = "The Texture",
        .dimensions = { static_cast<uint32_t>(x), static_cast<uint32_t>(y), 1 },
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .mipmapped = false,
    });

    // Cube vertex and index buffer
    // clang-format off
    const std::array<float, 40> vertices = {
        // pos[3], uv[2]
        -1.0F, -1.0F, -1.0F, 1.0F, 0.0F,
        -1.0F,  1.0F, -1.0F, 1.0F, 1.0F,
         1.0F,  1.0F, -1.0F, 0.0F, 1.0F,
         1.0F, -1.0F, -1.0F, 0.0F, 0.0F,

        -1.0F, -1.0F, 1.0F, 0.0F, 0.0F,
        -1.0F,  1.0F, 1.0F, 0.0F, 1.0F,
         1.0F,  1.0F, 1.0F, 1.0F, 1.0F,
         1.0F, -1.0F, 1.0F, 1.0F, 0.0F,
    };

    // CCW winding
    const std::array<uint32_t, 36> indices = {
        7, 5, 4, 7, 6, 5,   // +Z face (Front)
        0, 2, 3, 0, 1, 2,   // -Z face (Rear)
        3, 6, 7, 3, 2, 6,   // +X face (Right)
        4, 1, 0, 4, 5, 1,   // -X face (Left)
        6, 1 ,5, 6, 2, 1,   // +Y face (Top)
        3, 4, 0, 3, 7, 4,   // -Y face (Bottom)
    };
    // clang-format on

    const uint32_t indexBufferSizeBytes = indices.size() * sizeof(uint32_t);
    const Grace::BufferHandle indexBuffer = pDevice->CreateBuffer({
        .name = "Index Buffer",
        .allocSize = indexBufferSizeBytes,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .allocFlags = 0,
    });

    const uint32_t vertexBufferSizeBytes = vertices.size() * sizeof(float);
    const Grace::BufferHandle vertexBuffer = pDevice->CreateBuffer({
        .name = "Vertex Buffer",
        .allocSize = vertexBufferSizeBytes,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
               | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .allocFlags = 0,
    });

    const Grace::BufferHandle stagingBuffer = pDevice->CreateBuffer({
        .name = "Staging Buffer",
        .allocSize = textureSizeBytes + indexBufferSizeBytes + vertexBufferSizeBytes,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    });

    const uint32_t indexBufferStart = 0;
    pDevice->CopyMemoryToHostVisibleBuffer(stagingBuffer, indexBufferStart, indices.data(), indexBufferSizeBytes);

    const uint32_t vertexBufferStart = indexBufferStart + indexBufferSizeBytes;
    pDevice->CopyMemoryToHostVisibleBuffer(stagingBuffer, vertexBufferStart, vertices.data(), vertexBufferSizeBytes);

    const uint32_t textureStart = vertexBufferStart + vertexBufferSizeBytes;
    pDevice->CopyMemoryToHostVisibleBuffer(stagingBuffer, textureStart, data, textureSizeBytes);
    stbi_image_free(data);

    Grace::CommandBuffer& setupCmd = pDevice->BeginSingleTimeCommands();
    setupCmd.BeginDebugLabel("Frame Setup", { 1.0F, 0.28F, 0.3F, 1.0F });

    setupCmd.AddImageBarrier(texture, { Grace::AccessType::None }, { Grace::AccessType::CopyWrite });
    setupCmd.PipelineBarrier();

    // Copy image data to staging buffer, then copy staging buffer to texture image
    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = textureStart;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = { .width = static_cast<uint32_t>(x), .height = static_cast<uint32_t>(y), .depth = 1 };
    setupCmd.CopyBufferToImage(stagingBuffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { copyRegion });

    setupCmd.AddImageBarrier(texture, { Grace::AccessType::CopyWrite }, { Grace::AccessType::General });
    setupCmd.PipelineBarrier();

    // Copy indices data to staging buffer, then copy staging buffer to index buffer
    VkBufferCopy indexBufferCopyRegion = {};
    indexBufferCopyRegion.srcOffset = indexBufferStart;
    indexBufferCopyRegion.dstOffset = 0;
    indexBufferCopyRegion.size = indexBufferSizeBytes;
    setupCmd.CopyBuffer(stagingBuffer, indexBuffer, { indexBufferCopyRegion });

    // Copy vertices data to staging buffer, then copy staging buffer to vertex buffer
    VkBufferCopy vertexBufferCopyRegion = {};
    vertexBufferCopyRegion.srcOffset = vertexBufferStart;
    vertexBufferCopyRegion.dstOffset = 0;
    vertexBufferCopyRegion.size = vertexBufferSizeBytes;
    setupCmd.CopyBuffer(stagingBuffer, vertexBuffer, { vertexBufferCopyRegion });

    setupCmd.EndDebugLabel();
    pDevice->EndAndSubmitSingleTimeCommands();

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
    pDevice->UpdateBindlessDescriptorSet();

    while (!glfwWindowShouldClose(pWindow))
    {
        // Convert units to seconds
        float dt = cpuFrameTime * 0.001F;
        glfwPollEvents();

        model = glm::rotate(model, 1.0F * dt, glm::vec3(1.0F, 1.0F, 1.0F));
        view = HandleCamera(pWindow, cam, dt, cameraSpeed, cameraSensitivity);
        proj = glm::perspectiveFov(
            glm::radians(45.0f), static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.001F, 1000.0F);
        proj[1][1] *= -1.0f;
        mvp = proj * view * model;

        /* Prepare the frame */

        // The inFlightFences are created with signal bit, so they will already start signalled for the first use
        pDevice->WaitForFence(frame[frameIndex].inFlightFence);

        // Acquire an available image from the swapchain
        const Grace::FrameSyncGroup& fsg = pDevice->AcquireNextSwapchainImage({ windowWidth, windowHeight });

        // Reset the fence only when work has been submitted, otherwise next frame will be waiting on 'work'
        // to finish indefinitely
        pDevice->ResetFence(frame[frameIndex].inFlightFence);

        // Reset command pool, which will reset all command buffers allocated from it too
        frame[frameIndex].pCmdPool->Reset();

        // Now that the command buffer has been reset, we can start recording for the subsequent frame
        Grace::CommandBuffer& cmd = frame[frameIndex].cmd;
        cmd.BeginRecording();
        cmd.ResetQueryPoolFullRange<Grace::QueryType::Timestamp>(frameIndex);

        cmd.WriteTimestamp("GPU Frame Begin",
                           VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                           frameIndex,
                           Grace::QueryWriteFlags::WriteIfPreviousResultIsAvailable);

        /* Record commands */

        const Grace::ImageHandle& swapchainImg = pDevice->GetRecentlyAcquiredSwapchainImage();

        // Transition swapchain image to a writable layout
        cmd.AddImageBarrier(swapchainImg, { Grace::AccessType::None }, { Grace::AccessType::ClearWrite });
        cmd.PipelineBarrier();

        cmd.ClearColorImage(
            swapchainImg, { 0.35F, 0.55F, 0.85F, 1.0F }, { Grace::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT) });

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
        cmd.BindPipeline(texturedCubePH, VK_PIPELINE_BIND_POINT_GRAPHICS);
        cmd.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,
                               pDevice->GetPipelineLayout(pDevice->GetSolePipelineLayout()).GetVkPipelineLayout(),
                               0,
                               { pDevice->GetSoleDescriptorSet() });
        cmd.BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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
        cmd.PushConstants(
            pDevice->GetPipelineLayout(pDevice->GetSolePipelineLayout()).GetVkPipelineLayout(), sizeof(pc), &pc);

        cmd.WriteTimestamp("TexturedCube Pass Begin",
                           VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                           frameIndex,
                           Grace::QueryWriteFlags::WriteIfPreviousResultIsAvailable);
        cmd.DrawIndexed(indices.size(), 1, 0, 0, 0);
        cmd.WriteTimestamp("TexturedCube Pass End",
                           VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                           frameIndex,
                           Grace::QueryWriteFlags::WriteIfPreviousResultIsAvailable);

        cmd.EndDynamicRendering();
        cmd.EndDebugLabel();

        // Transition swapchain image to presentable layout
        cmd.AddImageBarrier(pDevice->GetRecentlyAcquiredSwapchainImage(),
                            { Grace::AccessType::ColorAttachmentReadWrite },
                            { Grace::AccessType::Present });
        cmd.PipelineBarrier();

        /* Wrap up the frame */

        cmd.WriteTimestamp("GPU Frame End",
                           VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                           frameIndex,
                           Grace::QueryWriteFlags::WriteIfPreviousResultIsAvailable);

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

        const Grace::TimestampQueryGroup& tqg = pDevice->GetQueryPoolResults<Grace::QueryType::Timestamp>(
            0, 0, VK_QUERY_RESULT_WITH_AVAILABILITY_BIT, frameIndex);

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
                .name = "Depth Image",
                .dimensions = { windowWidth, windowHeight, 1 },
                .format = VK_FORMAT_D32_SFLOAT,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .mipmapped = false,
            });
        }
        else if (ss == Grace::SwapchainStatus::Failure)
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

        frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
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
