#include <chrono>
#include <format>

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include <Grace/Grace.hpp>
#include <Grace/Ext/ShaderCompiler.hpp>

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
    glfwSetFramebufferSizeCallback(pWindow, [](GLFWwindow* pWindow, int width, int height) {
        bool& self = *static_cast<bool*>(glfwGetWindowUserPointer(pWindow));
        self = true;
    });

    VkSurfaceKHR surfaceKHR = nullptr;
    Grace::Context gpuContext(CONFIG_PATH);
    Grace::DebugReporter::Check(glfwCreateWindowSurface(gpuContext.GetInstance(), pWindow, nullptr, &surfaceKHR));
    Grace::Device* pDevice = gpuContext.DevicePtr(surfaceKHR);

    // Must first create the swapchain with desired extents
    pDevice->CreateSwapchain({ windowWidth, windowHeight }, vsync);

    // Grab a Command Pool for the Graphics Queue and allocate a command buffer from it
    Grace::CommandPool* pCmdPool = pDevice->GetCommandPool(Grace::QueueFamily::Graphics, "Example01::pCmdPool");
    Grace::CommandBuffer cmd = pCmdPool->GetOrAllocateCommandBuffer();

    const Grace::PipelineLayoutHandle helloTrianglePLH = pDevice->Create<Grace::PipelineLayout>({
        .flags = 0,
        .setLayouts = {},
        .pushConstantRanges = {},
    });

    // Create static pipeline from hello triangle shader
    Grace::GraphicsPipelineHandle helloTrianglePH = pDevice->Create<Grace::GraphicsPipeline>({
        .name = "Example01::helloTrianglePH",
        .shaders = {
            Grace::ShaderDesc(Grace::ShaderStage::Vertex, HelloTriangle_GRACE_SHADER_PROP_SPV_OUT_DIRECTORY "01_HelloTriangle.vert.spv"),
            Grace::ShaderDesc(Grace::ShaderStage::Fragment, HelloTriangle_GRACE_SHADER_PROP_SPV_OUT_DIRECTORY "01_HelloTriangle.frag.spv"),
        },
        .graphicsState = {
            .colourAttachmentFormats = { Grace::Format::RGBA8_SRGB },
            .topology = Grace::Topology::TriangleList,
            .polygonMode = Grace::PolygonMode::Fill,
            .cullMode = Grace::CullMode::Back,
            .frontFace = Grace::FrontFace::Clockwise,
        },
        .layout = helloTrianglePLH,
    });

    const Grace::FenceHandle inFlightFence = pDevice->Create<Grace::Fence>({
        .name = "Example01::inFlightFence",
        .flags = Grace::FenceFlags::CreateSignalled,
    });

    /* Render loop */
    std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

    bool compiling = false;

    while (!glfwWindowShouldClose(pWindow))
    {
        glfwPollEvents();

        if (glfwGetKey(pWindow, GLFW_KEY_C) == GLFW_PRESS && !compiling)
        {
            compiling = true;
            pDevice->WaitIdle();
            pDevice->Free<Grace::GraphicsPipeline>(helloTrianglePH);

            Grace::Ext::CompileShaderSingle("01_HelloTriangle.vert");

            helloTrianglePH = pDevice->Create<Grace::GraphicsPipeline>({
                .name = "Example01::helloTrianglePH",
                .shaders = {
                    Grace::ShaderDesc(Grace::ShaderStage::Vertex, "01_HelloTriangle.vert.spv"),
                    Grace::ShaderDesc(Grace::ShaderStage::Fragment, "01_HelloTriangle.frag.spv"),
                },
                .graphicsState = {
                    .colourAttachmentFormats = { Grace::Format::RGBA8_SRGB },
                    .topology = Grace::Topology::TriangleList,
                    .polygonMode = Grace::PolygonMode::Fill,
                    .cullMode = Grace::CullMode::Back,
                    .frontFace = Grace::FrontFace::Clockwise,
                },
                .layout = helloTrianglePLH,
            });
        }

        if (glfwGetKey(pWindow, GLFW_KEY_C) == GLFW_RELEASE && compiling)
        {
            compiling = false;
        }

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

        cmd.WriteTimestamp("GPU Frame Begin", Grace::PipelineStage::AllCommands, frameIndex);

        /* Record commands */

        const Grace::ImageHandle swapchainImg = pDevice->GetRecentlyAcquiredSwapchainImage();

        // Transition swapchain image to a writable layout
        cmd.AddImageBarrier(swapchainImg, { Grace::AccessType::None }, { Grace::AccessType::ClearWrite });
        cmd.PipelineBarrier();

        cmd.ClearColorImage(swapchainImg, Grace::Float4(0.35F, 0.55F, 0.85F, 1.0F));

        cmd.AddImageBarrier(
            swapchainImg, { Grace::AccessType::ClearWrite }, { Grace::AccessType::ColorAttachmentReadWrite });
        cmd.PipelineBarrier();

        cmd.BeginDebugLabel("Hello Triangle Pass");
        cmd.BeginDynamicRendering({
            .renderArea = { .extent = { windowWidth, windowHeight } },
            .colorAttachments = { Grace::ColourAttachmentInfo(pDevice->Get<Grace::Image>(swapchainImg), nullptr) },
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
        cmd.BindPipeline(helloTrianglePH);

        cmd.BeginQuery<Grace::QueryType::PipelineStatistics>("Hello Triangle Pipeline Stats", frameIndex);

        cmd.WriteTimestamp("Hello Triangle Pass Begin", Grace::PipelineStage::VertexShader, frameIndex);
        cmd.Draw(3, 1, 0, 0);
        cmd.WriteTimestamp("Hello Triangle Pass End", Grace::PipelineStage::FragmentShader, frameIndex);

        cmd.EndQuery<Grace::QueryType::PipelineStatistics>("Hello Triangle Pipeline Stats");

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
        pDevice->Submit(Grace::QueueFamily::Graphics, cmd, fsg, inFlightFence);

        // Present image as soon as it is safe to do so - when presentSemaphore is signalled
        const Grace::SwapchainStatus ss = pDevice->Present(fsg);

        const Grace::TimestampQueryGroup& tqg =
            pDevice->GetQueryPoolResults<Grace::QueryType::Timestamp>(0, 0, Grace::QueryResult::Wait);

        const Grace::PipelineStatsQueryGroup& psqg = pDevice->GetQueryPoolResults<Grace::QueryType::PipelineStatistics>(
            0, 0, Grace::QueryResult::WithAvailability);

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
