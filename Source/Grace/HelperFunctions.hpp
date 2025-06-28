#pragma once

#include <vector>
#include <filesystem>

#include <vulkan/vulkan.h>
#include <Grace/CommandGroup.hpp>
#include <Grace/Image.hpp>
#include <Grace/Buffer.hpp>
#include <Grace/Context.hpp>
#include <Grace/GraceExport.h>

namespace Grace
{

template <typename VK_HANDLE>
void AssignDebugName(VkDevice device, VK_HANDLE handle, VkObjectType type, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = type;
    nameInfo.objectHandle = uint64_t(handle);
    nameInfo.pObjectName = name;
    VK_SET_DEBUG_NAME(device, &nameInfo);
}

[[nodiscard]] GRACE_EXPORT VkRenderingAttachmentInfo ColourAttachmentInfo(
    const Image& image, VkClearValue* clear, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

[[nodiscard]] GRACE_EXPORT VkRenderingAttachmentInfo
DepthAttachmentInfo(const Image& image, VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

[[nodiscard]] GRACE_EXPORT VkRenderingInfo RenderingInfo(VkExtent2D renderArea,
                                                         uint32_t colourAttachmentCount,
                                                         const VkRenderingAttachmentInfo* pColourAttachments,
                                                         const VkRenderingAttachmentInfo* pDepthAttachment);

void CopyImageToImage(CommandBuffer cmd,
                      const Image& src,
                      const Image& dst,
                      VkExtent2D srcExtent,
                      VkExtent2D dstExtent,
                      uint32_t srcMipLevel = 0U,
                      uint32_t dstMipLevel = 0U);

[[nodiscard]] VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmdInfo,
                                       VkSemaphoreSubmitInfo* signalSemaphoreInfo,
                                       VkSemaphoreSubmitInfo* waitSemaphoreInfo);

void GenerateMipmaps(CommandBuffer& cmd, const Image& image);

[[nodiscard]] std::vector<char> ReadSpvFile(const std::filesystem::path& filename);

[[nodiscard]] bool
CreateShaderModule(VkDevice device, const std::filesystem::path& filename, VkShaderModule& shaderModule);

[[nodiscard]] VkPipelineShaderStageCreateInfo ShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module);

struct QueueFamilyIndices
{
    std::optional<uint32_t> transferFamily = {};
    std::optional<uint32_t> computeFamily = {};
    std::optional<uint32_t> graphicsFamily = {};
    std::optional<uint32_t> presentFamily = {};

    bool IsComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

[[nodiscard]] QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceKHR);

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats = {};
    std::vector<VkPresentModeKHR> presentModes = {};
};

[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceKHR);

} // namespace Grace
