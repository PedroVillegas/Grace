#include <chrono>
#include <format>

#include <GLFW/glfw3.h>
#include <Grace/Grace.hpp>

int main()
{
    uint64_t fragmentInvocations = 0;
    const bool vsync = true;
    bool framebufferHasResized = false;
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;

    glfwInit();
    const GLFWvidmode* vm = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_POSITION_X, (vm->width - windowWidth) / 2);
    glfwWindowHint(GLFW_POSITION_Y, (vm->height - windowHeight) / 2);
    GLFWwindow* pWindow = glfwCreateWindow(windowWidth, windowHeight, "Hello Triangle", nullptr, nullptr);
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
        .framesInFlight = 1,
        .queryGroupDesc = {
            .pipelineStatisticsFlags = VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT
                                     | VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT,
        },
        .pGlfwWindow = pWindow,
    };

    Grace::Context gpuContext({ .deviceConfig = deviceDesc });
    Grace::Device* pDevice = gpuContext.GetDevicePtr();

    // Must first create the swapchain with desired extents
    pDevice->CreateSwapchain({ windowWidth, windowHeight }, vsync);

    // Grab a Command Pool for the Graphics Queue and allocate a command buffer from it
    Grace::CommandPool* pCmdPool = pDevice->GetCommandPool(Grace::QueueFamily::Graphics, "Example01::pCmdPool");
    Grace::CommandBuffer cmd = pCmdPool->GetOrAllocateCommandBuffer();

    const VkFormat swapchainFormat = pDevice->GetSwapchainFormat();

    Grace::PipelineLayoutHandle helloTrianglePLH = pDevice->CreatePipelineLayout({
        .flags = 0,
        .setLayouts = {},
        .pushConstantRanges = {},
    });

    // Create pipeline from hello triangle shader
    Grace::PipelineBuilder pbuilder(pDevice);
    pbuilder.AddShader("01_HelloTriangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    pbuilder.AddShader("01_HelloTriangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    pbuilder.SetColourAttachmentFormat(&swapchainFormat);
    pbuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pbuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    pbuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pbuilder.SetMultisamplingNone();
    pbuilder.DisableBlending();
    pbuilder.DisableDepthTest();
    pbuilder.BuildGraphicsPipeline("Example01::helloTrianglePH", helloTrianglePLH);

    const Grace::PipelineHandle helloTrianglePH = pDevice->CreatePipeline(pbuilder.pipelineDesc);

    pDevice->FreePipelineLayout(helloTrianglePLH);

    const Grace::FenceHandle inFlightFence = pDevice->CreateFence({
        .name = "Example01::inFlightFence",
        .createFlags = VK_FENCE_CREATE_SIGNALED_BIT,
    });

    /* Render loop */
    std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(pWindow))
    {
        glfwPollEvents();

        /* Prepare the frame */

        const uint32_t frameIndex = pDevice->GetCurrentFrameInFlightIndex();

        // The inFlightFences are created with signal bit, so they will already start signalled for the first use
        pDevice->WaitForFence(inFlightFence);

        // Acquire an available image from the swapchain
        const Grace::FrameSyncGroup& fsg = pDevice->AcquireNextSwapchainImage({ windowWidth, windowHeight });

        // Reset the fence only when work has been submitted, otherwise next frame will be waiting on 'work'
        // to finish indefinitely
        pDevice->ResetFence(inFlightFence);

        // Reset command pool, which will reset all command buffers allocated from it too
        pCmdPool->Reset();

        // Now that the command buffer has been reset, we can start recording for the subsequent frame
        cmd.BeginRecording();
        cmd.ResetQueryPoolFullRange<Grace::QueryType::Timestamp>(frameIndex);
        cmd.ResetQueryPoolFullRange<Grace::QueryType::PipelineStatistics>(frameIndex);

        cmd.WriteTimestamp("GPU Frame Begin", VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frameIndex);

        /* Record commands */

        const Grace::ImageHandle swapchainImg = pDevice->GetRecentlyAcquiredSwapchainImage();

        // Transition swapchain image to a writable layout
        cmd.AddImageBarrier(swapchainImg, { Grace::AccessType::None }, { Grace::AccessType::ClearWrite });
        cmd.PipelineBarrier();

        cmd.ClearColorImage(swapchainImg,
                            { 0.35F, 0.55F, 0.85F, 1.0F },
                            { Grace::EntireImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT) });

        cmd.AddImageBarrier(
            swapchainImg, { Grace::AccessType::ClearWrite }, { Grace::AccessType::ColorAttachmentReadWrite });
        cmd.PipelineBarrier();

        cmd.BeginDebugLabel("Hello Triangle Pass");
        cmd.BeginDynamicRendering({
            .renderArea = { .extent = { windowWidth, windowHeight } },
            .colorAttachments = { Grace::ColourAttachmentInfo(pDevice->GetImage(swapchainImg), nullptr) },
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
        cmd.BindPipeline(helloTrianglePH, VK_PIPELINE_BIND_POINT_GRAPHICS);

        cmd.BeginQuery<Grace::QueryType::PipelineStatistics>("Hello Triangle Pipeline Stats", frameIndex);

        cmd.WriteTimestamp("Hello Triangle Pass Begin", VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, frameIndex);
        cmd.Draw(3, 1, 0, 0);
        cmd.WriteTimestamp("Hello Triangle Pass End", VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, frameIndex);

        cmd.EndQuery<Grace::QueryType::PipelineStatistics>("Hello Triangle Pipeline Stats");

        cmd.EndDynamicRendering();
        cmd.EndDebugLabel();

        // Transition swapchain image to presentable layout
        cmd.AddImageBarrier(pDevice->GetRecentlyAcquiredSwapchainImage(),
                            { Grace::AccessType::ColorAttachmentReadWrite },
                            { Grace::AccessType::Present });
        cmd.PipelineBarrier();

        /* Wrap up the frame */

        cmd.WriteTimestamp("GPU Frame End", VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frameIndex);

        // Finish recording for the command buffer for this frame
        cmd.EndRecording();

        // Submit the command buffer
        // When using the swapchain, the device needs to be certain that, at this point,
        // there is a swapchain image available since it will be writing to it - it will
        // wait on the acquireSemaphore to be signalled
        // Once the submission is complete, one can be sure that the swapchain image is no
        // longer being written to, so presentSemaphore is signalled
        pDevice->Submit(Grace::QueueFamily::Graphics, cmd, fsg, inFlightFence);

        // Present image as soon as it is safe to do so - when presentSemaphore is signalled
        const Grace::SwapchainStatus ss = pDevice->Present(fsg);

        const Grace::TimestampQueryGroup& tqg =
            pDevice->GetQueryPoolResults<Grace::QueryType::Timestamp>(0, 0, VK_QUERY_RESULT_WAIT_BIT);

        const Grace::PipelineStatsQueryGroup& psqg = pDevice->GetQueryPoolResults<Grace::QueryType::PipelineStatistics>(
            0, 0, VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

        float helloTrianglePassTime =
            tqg.Duration<Grace::TimestampUnits::Milliseconds>("Hello Triangle Pass Begin", "Hello Triangle Pass End");

        float gpuFrameTime = tqg.Duration<Grace::TimestampUnits::Milliseconds>("GPU Frame Begin", "GPU Frame End");

        psqg.GetQueryIfAvailable(fragmentInvocations, "Hello Triangle Pipeline Stats", 1);

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

        const std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        float cpuFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count() / 1000.0F;
        lastTime = now;

        const std::string windowTitle = std::format(
            "CPU Frame Time: {}ms | GPU Frame Time: {}ms | Hello Triangle Pass: {}ms | Fragment Invocations: {}",
            cpuFrameTime,
            gpuFrameTime,
            helloTrianglePassTime,
            fragmentInvocations);
        glfwSetWindowTitle(pWindow, windowTitle.c_str());

        pDevice->AdvanceToNextFrame();
    }

    glfwTerminate();
    return 0;
}
