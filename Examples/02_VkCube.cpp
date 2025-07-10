#include <chrono>
#include <format>
#include <iostream>

#include <GLFW/glfw3.h>
#include <Grace/Grace.hpp>

struct FrameData
{
    Grace::CommandPool* pCmdPool = nullptr;
    Grace::CommandBuffer cmd = {};
    Grace::FenceHandle inFlightFence = {};
};

int main()
{
    constexpr uint32_t FRAMES_IN_FLIGHT = 2;
    uint32_t frameIndex = 0;

    // Timings
    double vkCubePassTime = 0.0;
    double gpuFrameTime = 0.0;

    bool vsync = true;
    bool framebufferHasResized = false;
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* pWindow = glfwCreateWindow(windowWidth, windowHeight, "VkCube", nullptr, nullptr);
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

    const VkFormat swapchainFormat = pDevice->GetSwapchainFormat();

    Grace::PipelineLayoutHandle helloTrianglePLH = pDevice->CreatePipelineLayout({
        .flags = 0,
        .setLayouts = {},
        .pushConstantRanges = {},
    });

    // Create pipeline from hello triangle shader
    Grace::PipelineBuilder pbuilder(pDevice);
    pbuilder.AddShader("helloTriangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    pbuilder.AddShader("helloTriangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    pbuilder.SetColourAttachmentFormat(&swapchainFormat);
    pbuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pbuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    pbuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pbuilder.SetMultisamplingNone();
    pbuilder.DisableBlending();
    pbuilder.DisableDepthTest();
    pbuilder.BuildGraphicsPipeline("Hello Triangle Pipeline", pDevice->GetPipelineLayout(helloTrianglePLH));

    const Grace::PipelineHandle helloTrianglePH = pDevice->CreatePipeline(pbuilder.pipelineDesc);

    /* Render loop */
    std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(pWindow))
    {
        glfwPollEvents();

        /* Prepare the frame */

        // The inFlightFences are created with signal bit, so they will already start signalled for the first use
        pDevice->WaitForFence(pDevice->GetFence(frame[frameIndex].inFlightFence));

        // Acquire an available image from the swapchain
        const Grace::FrameSyncGroup& fsg = pDevice->AcquireNextSwapchainImage({ windowWidth, windowHeight });

        // Reset the fence only when work has been submitted, otherwise next frame will be waiting on 'work'
        // to finish indefinitely
        pDevice->ResetFence(pDevice->GetFence(frame[frameIndex].inFlightFence));

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

        const Grace::Image& swapchainImg = pDevice->GetRecentlyAcquiredSwapchainImage();

        // Transition swapchain image to a writable layout
        cmd.AddImageBarrier(swapchainImg, { Grace::AccessType::None }, { Grace::AccessType::ClearWrite });
        cmd.PipelineBarrier();

        cmd.ClearColorImage(
            swapchainImg, { 0.35F, 0.55F, 0.85F, 1.0F }, { Grace::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT) });

        cmd.AddImageBarrier(
            swapchainImg, { Grace::AccessType::ClearWrite }, { Grace::AccessType::ColorAttachmentReadWrite });
        cmd.PipelineBarrier();

        cmd.BeginDebugLabel("VkCube Pass");
        cmd.BeginDynamicRendering({
            .renderArea = { .extent = { windowWidth, windowHeight } },
            .colorAttachments = { Grace::ColourAttachmentInfo(swapchainImg, nullptr) },
        });

        cmd.SetViewport({ {
            .x = 0,
            .y = 0,
            .width = static_cast<float>(windowWidth),
            .height = static_cast<float>(windowHeight),
            .minDepth = 0.0F,
            .maxDepth = 1.0F,
        } });

        cmd.SetScissor({ {
            .offset = { 0, 0 },
            .extent = { windowWidth, windowHeight },
        } });

        // Bind helloTriangle pipeline and execute a draw call
        cmd.BindGraphicsPipeline(pDevice->GetPipeline(helloTrianglePH));

        cmd.WriteTimestamp("VkCube Pass Begin",
                           VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                           frameIndex,
                           Grace::QueryWriteFlags::WriteIfPreviousResultIsAvailable);
        cmd.Draw(3, 1, 0, 0);
        cmd.WriteTimestamp("VkCube Pass End",
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
        pDevice->Submit(Grace::QueueFamily::Graphics, cmd, fsg, pDevice->GetFence(frame[frameIndex].inFlightFence));

        // Present image as soon as it is safe to do so - when presentSemaphore is signalled
        const Grace::SwapchainStatus ss = pDevice->Present(fsg.presentSemaphore, fsg.imageIndex);

        pDevice->WaitIdle();

        const Grace::TimestampQueryGroup& tqg = pDevice->GetQueryPoolResults<Grace::QueryType::Timestamp>(
            0, 0, VK_QUERY_RESULT_WITH_AVAILABILITY_BIT, frameIndex);

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(pDevice->GetPhysicalDevice(), &props);

        tqg.DurationIfAvailable<Grace::TimestampUnits::Milliseconds>(
            vkCubePassTime, "VkCube Pass Begin", "VkCube Pass End", frameIndex);

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
        }
        else if (ss == Grace::SwapchainStatus::Failure)
        {
            break;
        }

        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        float cpuFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count() / 1000.0F;
        lastTime = now;

        std::string windowTitle = std::format("CPU Frame Time: {}ms | GPU Frame Time: {}ms | VkCube Pass: {}ms",
                                              cpuFrameTime,
                                              static_cast<float>(gpuFrameTime),
                                              static_cast<float>(vkCubePassTime));
        glfwSetWindowTitle(pWindow, windowTitle.c_str());

        frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
    }

    glfwTerminate();
    return 0;
}
