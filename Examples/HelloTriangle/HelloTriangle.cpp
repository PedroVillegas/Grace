#include <iostream>

#include <GLFW/glfw3.h>
#include <Grace/Grace.hpp>

int main()
{
    bool framebufferHasResized = false;
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
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
        .enableVsync = true,
        .enableValidationLayers = true,
        .pGlfwWindow = pWindow,
    };

    Grace::Context gpuContext({ .deviceConfig = deviceDesc });
    Grace::Device* pDevice = gpuContext.GetDevicePtr();

    // Must first create the swapchain with desired extents
    pDevice->CreateSwapchain({ windowWidth, windowHeight });

    // Grab a Command Pool for the Graphics Queue and allocate a command buffer from it
    Grace::CommandPool* pCmdPool = pDevice->GetCommandPool(Grace::QueueFamily::Graphics, "Command pool");
    Grace::CommandBuffer cmd = pCmdPool->GetOrAllocateCommandBuffer();

    const VkFormat swapchainFormat = pDevice->GetSwapchain().GetFormat();

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

    pDevice->FreePipelineLayout(helloTrianglePLH);

    /* Render loop */

    while (!glfwWindowShouldClose(pWindow))
    {
        glfwPollEvents();

        /* Prepare the frame */

        // The inFlightFences are created with signal bit, so they will already start signalled for the first use
        pDevice->WaitForFences({ pDevice->GetRecentImageAcquiredDesc().inFlightFence });

        // Acquire an available image from the swapchain
        const Grace::FrameSyncGroup& fsg = pDevice->AcquireNextSwapchainImage({ windowWidth, windowHeight });

        // Reset the fence only when work has been submitted, otherwise next frame will be waiting on 'work'
        // to finish indefinitely
        pDevice->ResetFences({ fsg.inFlightFence });

        // Reset command pool, which will reset all command buffers allocated from it too
        pCmdPool->Reset();

        // Now that the command buffer has been reset, we can start recording for the subsequent frame
        cmd.BeginRecording();

        /* Record commands */

        const Grace::Image& swapchainImg = pDevice->GetSwapchain().GetRecentAcquiredImage();

        // Transition swapchain image to a writable layout
        cmd.AddImageBarrier(swapchainImg, { Grace::AccessType::None }, { Grace::AccessType::ClearWrite });
        cmd.PipelineBarrier();

        cmd.ClearColorImage(
            swapchainImg, { 0.35F, 0.55F, 0.85F, 1.0F }, { Grace::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT) });

        cmd.AddImageBarrier(
            swapchainImg, { Grace::AccessType::ClearWrite }, { Grace::AccessType::ColorAttachmentReadWrite });
        cmd.PipelineBarrier();

        cmd.BeginDebugLabel("Hello Triangle Pass");
        cmd.BeginDynamicRendering({
            .renderArea = { .extent = { windowWidth, windowHeight } },
            .colorAttachments = { Grace::ColourAttachmentInfo(swapchainImg, nullptr) },
        });

        const std::vector<VkViewport> viewports = { {
            .x = 0,
            .y = 0,
            .width = static_cast<float>(windowWidth),
            .height = static_cast<float>(windowHeight),
            .minDepth = 0.0F,
            .maxDepth = 1.0F,
        } };
        cmd.SetViewport(viewports);

        const std::vector<VkRect2D> scissors = { {
            .offset = { 0, 0 },
            .extent = { windowWidth, windowHeight },
        } };
        cmd.SetScissor(scissors);

        // Bind helloTriangle pipeline and execute a draw call
        cmd.BindGraphicsPipeline(pDevice->GetPipeline(helloTrianglePH));
        cmd.Draw(3, 1, 0, 0);

        cmd.EndDynamicRendering();
        cmd.EndDebugLabel();

        // Transition swapchain image to presentable layout
        cmd.AddImageBarrier(pDevice->GetSwapchain().GetRecentAcquiredImage(),
                            { Grace::AccessType::ColorAttachmentReadWrite },
                            { Grace::AccessType::Present });
        cmd.PipelineBarrier();

        /* Wrap up the frame */

        // Finish recording for the command buffer for this frame
        cmd.EndRecording();

        // Submit the command buffer
        // When using the swapchain, the device needs to be certain that, at this point,
        // there is a swapchain image available since it will be writing to it - it will
        // wait on the acquireSemaphore to be signalled
        // Once the submission is complete, one can be sure that the swapchain image is no
        // longer being written to, so presentSemaphore is signalled
        pDevice->Submit(Grace::QueueFamily::Graphics,
                        { cmd },
                        { fsg.acquireSemaphore },
                        { fsg.presentSemaphore },
                        fsg.inFlightFence);

        // Present image as soon as it is safe to do so - when presentSemaphore is signalled
        const Grace::SwapchainStatus ss = pDevice->Present(fsg.presentSemaphore, fsg.imageIndex);

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

            pDevice->CreateSwapchain({ windowWidth, windowHeight });
        }
        else if (ss == Grace::SwapchainStatus::Failure)
        {
            break;
        }
    }

    glfwTerminate();
    return 0;
}
