#include <chrono>
#include <format>

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include <Grace/Grace.hpp>
#include <Grace/Ext/ShaderCompiler.hpp>

int main()
{
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
    GLFWwindow* pWindow = glfwCreateWindow(windowWidth, windowHeight, "Simple Compute Shader", nullptr, nullptr);
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
        .framesInFlight = 1,
        .requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME },
        .queryGroupDesc = {
            .pipelineStatisticsFlags = Grace::QueryStats::ComputeShaderInvocations,
        },
        .surfacekhr = surfaceKHR,
    });

    // Must first create the swapchain with desired extents
    pDevice->CreateSwapchain({ windowWidth, windowHeight }, vsync);

    // Grab a Command Pool for the Graphics Queue and allocate a command buffer from it
    Grace::CommandPool* pCmdPool = pDevice->GetCommandPool(Grace::QueueFamily::Graphics, "Example03::pCmdPool");
    Grace::CommandBuffer cmd = pCmdPool->GetOrAllocateCommandBuffer();

    Grace::PipelineHandle simpleComputeShaderPipeline = pDevice->CreateComputePipeline({
        .name = "Example03::simpleComputeShaderPipeline",
        .shader = Grace::ShaderDesc(Grace::ShaderStage::Compute, "03_ComputeShader.slang.spv"),
        .layout = pDevice->GetSolePipelineLayout(),
    });

    const Grace::FenceHandle inFlightFence = pDevice->CreateFence({
        .name = "Example03::inFlightFence",
        .flags = Grace::FenceFlags::CreateSignalled,
    });

    Grace::ImageHandle renderImage = pDevice->CreateImage({
        .name = "Example03::renderImage",
        .dimensions = { static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight), 1 },
        .format = Grace::Format::RGBA8_UNorm,
        .usage = Grace::ImageUsage::StorageImage | Grace::ImageUsage::TransferSrc,
        .access = Grace::AccessType::General,
        .size = 0,
        .data = nullptr,
        .mipmapped = false,
    });

    /* Render loop */
    std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

    bool compiling = false;

    while (!glfwWindowShouldClose(pWindow))
    {
        glfwPollEvents();

        if (glfwGetKey(pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS && !compiling)
        {
            compiling = true;
            pDevice->WaitIdle();
            pDevice->FreePipeline(simpleComputeShaderPipeline);

            Grace::Ext::CompileShaderSingle("03_ComputeShader.slang");

            simpleComputeShaderPipeline = pDevice->CreateComputePipeline({
                .name = "Example03::simpleComputeShaderPipeline",
                .shader = Grace::ShaderDesc(Grace::ShaderStage::Compute, "03_ComputeShader.slang.spv"),
                .layout = pDevice->GetSolePipelineLayout(),
            });
        }

        if (glfwGetKey(pWindow, GLFW_KEY_ESCAPE) == GLFW_RELEASE && compiling)
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

        pDevice->UpdateBindlessDescriptorSet();

        // Now that the command buffer has been reset, we can start recording for the subsequent frame
        cmd.BeginRecording();
        cmd.ResetQueryPoolFullRange<Grace::QueryType::Timestamp>(frameIndex);

        cmd.WriteTimestamp("GPU Frame Begin", Grace::PipelineStage::AllCommands, frameIndex);

        /* Record commands */

        Grace::ImageHandle swapchainImg = pDevice->GetRecentlyAcquiredSwapchainImage();

        cmd.AddImageBarrier(swapchainImg, { Grace::AccessType::None }, { Grace::AccessType::BlitWrite });
        cmd.PipelineBarrier();

        cmd.BeginDebugLabel("Simple Compute Shader Pass");

        struct PushConsts
        {
            uint32_t TextureId;
        } pc;

        pc.TextureId = pDevice->GetImage(renderImage).GetStorageImgId();

        cmd.PushConstants(pDevice->GetSolePipelineLayout(), sizeof(pc), &pc);
        cmd.BindDescriptorSets(
            Grace::PipelineBindPoint::Compute, pDevice->GetSolePipelineLayout(), { pDevice->GetSoleDescriptorSet() });
        cmd.BindPipeline(simpleComputeShaderPipeline);

        cmd.WriteTimestamp("Simple Compute Shader Pass Begin", Grace::PipelineStage::ComputeShader, frameIndex);
        cmd.Dispatch(static_cast<uint32_t>(windowWidth / 16.0F) + 1, static_cast<uint32_t>(windowHeight / 16.0F) + 1);
        cmd.WriteTimestamp("Simple Compute Shader Pass End", Grace::PipelineStage::ComputeShader, frameIndex);

        cmd.EndDebugLabel();

        cmd.AddMemoryBarrier({ Grace::AccessType::ComputeShaderWrite }, { Grace::AccessType::BlitRead });
        cmd.PipelineBarrier();

        const VkImageBlit2 blit = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
            .pNext = nullptr,
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcOffsets = {
                VkOffset3D(0, 0, 0),
                VkOffset3D(windowWidth, windowHeight, 1),
            },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffsets = {
                VkOffset3D(0, 0, 0),
                VkOffset3D(windowWidth, windowHeight, 1),
            },
        };

        cmd.BlitImage({
            .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
            .pNext = nullptr,
            .srcImage = pDevice->GetImage(renderImage).GetImage(),
            .srcImageLayout = VK_IMAGE_LAYOUT_GENERAL,
            .dstImage = pDevice->GetImage(swapchainImg).GetImage(),
            .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .regionCount = 1,
            .pRegions = &blit,
            .filter = VK_FILTER_LINEAR,
        });

        cmd.AddImageBarrier(pDevice->GetRecentlyAcquiredSwapchainImage(),
                            { Grace::AccessType::BlitWrite },
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

        float helloTrianglePassTime = tqg.Duration<Grace::TimestampUnits::Milliseconds>(
            "Simple Compute Shader Pass Begin", "Simple Compute Shader Pass End");

        float gpuFrameTime = tqg.Duration<Grace::TimestampUnits::Milliseconds>("GPU Frame Begin", "GPU Frame End");

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
            pDevice->FreeImage(renderImage);

            renderImage = pDevice->CreateImage({
                .name = "Example03::renderImage",
                .dimensions = { static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight), 1 },
                .format = Grace::Format::RGBA8_UNorm,
                .usage = Grace::ImageUsage::StorageImage | Grace::ImageUsage::TransferSrc,
                .access = Grace::AccessType::General,
                .size = 0,
                .data = nullptr,
                .mipmapped = false,
            });
        } else if (ss == Grace::SwapchainStatus::Failure)
        {
            break;
        }

        const std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        float cpuFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count() / 1000.0F;
        lastTime = now;

        const std::string windowTitle =
            std::format("CPU Frame Time: {}ms | GPU Frame Time: {}ms | Hello Triangle Pass: {}ms",
                        cpuFrameTime,
                        gpuFrameTime,
                        helloTrianglePassTime);
        glfwSetWindowTitle(pWindow, windowTitle.c_str());
    }

    glfwTerminate();
    return 0;
}
