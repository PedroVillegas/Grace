#include <Grace/HelperFunctions.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/Detail/Assert.hpp>

#include <fstream>
#include <iostream>

namespace Grace
{

VkImageSubresourceRange EntireImageSubresourceRange(ImageAspect aspect)
{
    return {
        .aspectMask = static_cast<VkImageAspectFlags>(aspect),
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };
}

VkImageAspectFlags DetermineImageAspectFlagsFromFormat(VkFormat format)
{
    // clang-format off
    switch (format)
    {
    case VK_FORMAT_D16_UNORM: GRACE_FALLTHROUGH;
    case VK_FORMAT_D32_SFLOAT:
        return VK_IMAGE_ASPECT_DEPTH_BIT;

    case VK_FORMAT_D16_UNORM_S8_UINT: GRACE_FALLTHROUGH;
    case VK_FORMAT_D24_UNORM_S8_UINT: GRACE_FALLTHROUGH;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

    case VK_FORMAT_S8_UINT:
        return VK_IMAGE_ASPECT_STENCIL_BIT;

    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
    // clang-format on
}

VkRenderingAttachmentInfo ColourAttachmentInfo(const Image& image, VkClearValue* clear, VkImageLayout imageLayout)
{
    GRACE_ASSERT(!image.Exists());

    VkClearValue cl = {};
    if (clear)
    {
        cl = *clear;
    }

    VkRenderingAttachmentInfo colourAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = image.GetDefaultView().VkHandle(),
        .imageLayout = imageLayout,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = nullptr,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = cl,
    };

    return colourAttachment;
}

VkRenderingAttachmentInfo DepthAttachmentInfo(const Image& image, VkImageLayout imageLayout)
{
    GRACE_ASSERT(!image.Exists());

    VkRenderingAttachmentInfo depthAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = image.GetDefaultView().VkHandle(),
        .imageLayout = imageLayout,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = nullptr,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = { .depthStencil = { .depth = 0.0F } },
    };

    return depthAttachment;
}

VkRenderingInfo RenderingInfo(VkExtent2D renderArea,
                              uint32_t colourAttachmentCount,
                              const VkRenderingAttachmentInfo* pColourAttachments,
                              const VkRenderingAttachmentInfo* pDepthAttachment)
{
    // pColourAttachments and pDepthAttachment CAN be nullptrs
    return VkRenderingInfo(
        { .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
          .pNext = nullptr,
          .renderArea = VkRect2D { { 0, 0 }, { (uint32_t) renderArea.width, (uint32_t) renderArea.height } },
          .layerCount = 1,
          .colorAttachmentCount = colourAttachmentCount,
          .pColorAttachments = pColourAttachments,
          .pDepthAttachment = pDepthAttachment });
}

VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmdInfo,
                         VkSemaphoreSubmitInfo* signalSemaphoreInfo,
                         VkSemaphoreSubmitInfo* waitSemaphoreInfo)
{
    GRACE_ASSERT(cmdInfo != nullptr);

    VkSubmitInfo2 submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = nullptr,
        .flags = 0,
        .waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0U : 1U,
        .pWaitSemaphoreInfos = waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = cmdInfo,
        .signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0U : 1U,
        .pSignalSemaphoreInfos = signalSemaphoreInfo,
    };

    return submitInfo;
}

VkPipelineShaderStageCreateInfo ShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module)
{
    GRACE_ASSERT(module != nullptr);

    return VkPipelineShaderStageCreateInfo({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                             .pNext = nullptr,
                                             .stage = stage,
                                             .module = module,
                                             .pName = "main" });
}

void CreateShaderModule(VkDevice device, const std::filesystem::path& filename, VkShaderModule& shaderModule)
{
    GRACE_ASSERT(device != nullptr);
    GRACE_ASSERT(std::filesystem::exists(filename));

    auto shaderCode = ReadSpvFile(filename);

    VkShaderModuleCreateInfo smci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = shaderCode.size(),
        .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data()),
    };

    DebugReporter::Check(vkCreateShaderModule(device, &smci, nullptr, &shaderModule));
    const std::string shaderModuleDebugName = filename.filename().string();
    AssignDebugName<VkShaderModule>(device, shaderModule, shaderModuleDebugName.c_str());
}

std::vector<char> ReadSpvFile(const std::filesystem::path& filename)
{
    GRACE_ASSERT(std::filesystem::exists(filename));

    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        std::cout << "Failed to open file!\n";
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceKHR)
{
    GRACE_ASSERT(physicalDevice != nullptr);

    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        if (surfaceKHR != nullptr)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surfaceKHR, &presentSupport);

            if (presentSupport)
            {
                indices.presentFamily = i;
            }
        }
        i++;
    }

    return indices;
}

SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceKHR)
{
    GRACE_ASSERT(physicalDevice != nullptr);

    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surfaceKHR, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceKHR, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceKHR, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaceKHR, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, surfaceKHR, &presentModeCount, details.presentModes.data());
    }

    return details;
}

} // namespace Grace
