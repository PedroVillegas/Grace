#include <iostream>
#include <set>

#include <GLFW/glfw3.h>
#include <Grace/Grace.hpp>

int main()
{
    uint32_t width = 800, height = 600;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* pWindow = glfwCreateWindow(width, height, "Hello Triangle", nullptr, nullptr);

    Grace::Context gpuContext;

    Grace::DeviceDesc deviceDesc = {
        .maxImageDescriptors = 65535,
        .maxSamplerDescriptors = 65535,
        .maxBufferDescriptors = 65535,
        .enableVsync = true,
        .enableValidationLayers = true,
        .pGlfwWindow = pWindow,
    };

    gpuContext.Initialise({ .deviceConfig = deviceDesc });
    Grace::Device* pDevice = gpuContext.GetDevicePtr();

    // Must first create the swapchain with desired extents
    pDevice->CreateSwapchain({ width, height });

    // Grab a Command Pool for the Graphics Queue and allocate a command buffer from it
    Grace::CommandPool* pCmdPool = pDevice->GetCommandPool(Grace::QueueFamily::Graphics, "Command pool");
    Grace::CommandBuffer cmd = pCmdPool->GetOrAllocateCommandBuffer(pDevice);

    VkFormat swapchainFormat = pDevice->GetSwapchain().GetFormat();

    Grace::PipelineLayoutHandle helloTrianglePLH = pDevice->CreatePipelineLayout({
        .flags = 0,
        .setLayouts = {},
        .pushConstantRanges = {},
    });

    // Create pipeline from hello triangle shader
    Grace::PipelineBuilder pbuilder(pDevice->GetVkDevice());
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

    Grace::PipelineHandle helloTrianglePH = pDevice->CreatePipeline(pbuilder.pipelineDesc);

    pDevice->FreePipelineLayout(helloTrianglePLH);

    /* Render loop */

    while (!glfwWindowShouldClose(pWindow))
    {
        glfwPollEvents();

        /* Prepare the frame */

        // The inFlightFences are created with signal bit, so they will already start signalled for the first use
        pDevice->WaitForFences({ pDevice->GetRecentImageAcquiredDesc().inFlightFence });

        // Acquire an available image from the swapchain
        Grace::FrameSyncGroup& fsg = pDevice->AcquireNextSwapchainImage({ width, height });

        // Reset the fence only when work has been submitted, otherwise next frame will be waiting on 'work'
        // to finish indefinitely
        pDevice->ResetFences({ fsg.inFlightFence });

        // Reset command pool, which will reset all command buffers allocated from it too
        pCmdPool->Reset(pDevice);

        // Now that the command buffer has been reset, we can start recording for the subsequent frame
        cmd.BeginRecording();

        /* Record commands */

        const Grace::Image& swapchainImg = pDevice->GetSwapchain().GetRecentAcquiredImage();

        // Transition swapchain image to a writable layout
        cmd.CmdAddImageLayoutTransition(pDevice->GetSwapchain().GetRecentAcquiredImage(),
                                        THSVS_ACCESS_NONE,
                                        THSVS_ACCESS_GENERAL,
                                        VK_IMAGE_ASPECT_COLOR_BIT);
        cmd.CmdExecuteBarriers();

        VkClearValue clear = { .color = { 0.35F, 0.55F, 0.85F, 1.0F } };
        cmd.CmdBeginDynamicRendering({
            .renderArea = { width, height },
            .colorAttachments = { Grace::ColourAttachmentInfo(swapchainImg, &clear) },
        });

        std::vector<VkViewport> viewports = { {
            .x = 0,
            .y = 0,
            .width = static_cast<float>(width),
            .height = static_cast<float>(height),
            .minDepth = 0.0F,
            .maxDepth = 1.0F,
        } };
        cmd.CmdSetViewport(viewports);

        std::vector<VkRect2D> scissors = { {
            .offset = { 0, 0 },
            .extent = { width, height },
        } };
        cmd.CmdSetScissor(scissors);

        // Bind helloTriangle pipeline and execute a draw call
        cmd.CmdBindGraphicsPipeline(pDevice->GetPipeline(helloTrianglePH));
        cmd.CmdDraw(3, 1, 0, 0);

        cmd.CmdEndDynamicRendering();

        // Transition swapchain image to presentable layout
        cmd.CmdAddImageLayoutTransition(pDevice->GetSwapchain().GetRecentAcquiredImage(),
                                        THSVS_ACCESS_GENERAL,
                                        THSVS_ACCESS_PRESENT,
                                        VK_IMAGE_ASPECT_COLOR_BIT);
        cmd.CmdExecuteBarriers();

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
        Grace::SwapchainStatus ss = pDevice->Present(fsg.presentSemaphore, fsg.imageIndex);

        // if (ss == SwapchainStatus::ShouldResize || bFrameBufferResized)
        // {
        //     bFrameBufferResized = false;
        //
        //     // Handle minimisation
        //     int width = 0, height = 0;
        //     glfwGetFramebufferSize(window.GetGLFWwindow(), &width, &height);
        //     while (width == 0 || height == 0)
        //     {
        //         glfwGetFramebufferSize(window.GetGLFWwindow(), &width, &height);
        //         glfwWaitEvents();
        //     }
        //
        //     window.SetViewportWidth(width);
        //     window.SetViewportHeight(height);
        //
        //     device->CreateSwapchain({ window.GetViewportWidth(), window.GetViewportHeight() });
        // }
        // else if (ss == SwapchainStatus::Failure)
        // {
        //     LOG_ERROR("Failed to present swap chain image!");
        // }
    }

    glfwTerminate();
    return 0;
}
