#pragma once

#include <vector>
#include <memory>

#include <vulkan/vulkan.h>
#include <Grace/thsvs_simpler_vulkan_synchronization.h>

namespace Grace
{

class Image;

[[nodiscard]] VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);

class BarrierBuilder
{
public:
    BarrierBuilder() = default;
    ~BarrierBuilder() = default;

    BarrierBuilder& ExecutePipelineBarrier(VkCommandBuffer cmd);

    BarrierBuilder& AddMemoryBarrier(ThsvsAccessType prevAccess, ThsvsAccessType nextAccess);

    BarrierBuilder& AddImageLayoutTransition(const Image& image,
                                             ThsvsAccessType prevAccess,
                                             ThsvsAccessType nextAccess,
                                             VkImageAspectFlags aspectMask);

    BarrierBuilder& AddImageLayoutTransition(VkImage image,
                                             ThsvsAccessType prevAccess,
                                             ThsvsAccessType nextAccess,
                                             VkImageAspectFlags aspectMask);

private:
    struct ImageBarrierDesc
    {
        VkImage image;
        ThsvsAccessType prevAccess;
        ThsvsAccessType nextAccess;
        VkImageAspectFlags aspectMask;
    };

    struct MemoryBarrierDesc
    {
        ThsvsAccessType prevAccess;
        ThsvsAccessType nextAccess;
    };

    std::vector<ImageBarrierDesc> m_ImageBarrierDescs = {};
    std::vector<MemoryBarrierDesc> m_MemoryBarrierDescs = {};
};

} // namespace Grace
