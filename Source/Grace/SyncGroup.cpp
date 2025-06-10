#include "SyncGroup.hpp"

#include <cassert>

#include <Grace/DebugReporter.hpp>
#include <Grace/Image.hpp>

#include "CommandGroup.hpp"

namespace Grace
{

VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask)
{
    VkImageSubresourceRange subImage = {};
    subImage.aspectMask = aspectMask;
    subImage.baseMipLevel = 0;
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount = 1;

    return subImage;
}

BarrierBuilder& BarrierBuilder::ExecutePipelineBarrier(VkCommandBuffer cmd)
{
    std::vector<ThsvsAccessType> memoryBarrierPrevAccesses(m_MemoryBarrierDescs.size());
    std::vector<ThsvsAccessType> memoryBarrierNextAccesses(m_MemoryBarrierDescs.size());

    for (uint32_t i = 0; i < m_MemoryBarrierDescs.size(); ++i)
    {
        if (m_MemoryBarrierDescs[i].prevAccess != THSVS_ACCESS_NONE)
        {
            memoryBarrierPrevAccesses.emplace_back(m_MemoryBarrierDescs[i].prevAccess);
        }

        if (m_MemoryBarrierDescs[i].nextAccess != THSVS_ACCESS_NONE)
        {
            memoryBarrierNextAccesses.emplace_back(m_MemoryBarrierDescs[i].nextAccess);
        }
    }

    ThsvsGlobalBarrier globalBarrier;
    globalBarrier.prevAccessCount = static_cast<uint32_t>(memoryBarrierPrevAccesses.size());
    globalBarrier.pPrevAccesses = memoryBarrierPrevAccesses.data();
    globalBarrier.nextAccessCount = static_cast<uint32_t>(memoryBarrierNextAccesses.size());
    globalBarrier.pNextAccesses = memoryBarrierNextAccesses.data();

    std::vector<ThsvsImageBarrier> imageBarriers(m_ImageBarrierDescs.size());

    for (uint32_t i = 0; i < m_ImageBarrierDescs.size(); ++i)
    {
        ThsvsImageBarrier barrier;
        barrier.image = m_ImageBarrierDescs[i].image;
        barrier.prevAccessCount = 1;
        barrier.pPrevAccesses = &m_ImageBarrierDescs[i].prevAccess;
        barrier.nextAccessCount = 1;
        barrier.pNextAccesses = &m_ImageBarrierDescs[i].nextAccess;
        barrier.prevLayout = THSVS_IMAGE_LAYOUT_OPTIMAL;
        barrier.nextLayout = THSVS_IMAGE_LAYOUT_OPTIMAL;
        barrier.subresourceRange = ImageSubresourceRange(m_ImageBarrierDescs[i].aspectMask);
        barrier.discardContents = false;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        imageBarriers[i] = barrier;
    }

    thsvsCmdPipelineBarrier(cmd,
                            &globalBarrier,
                            0,
                            nullptr,
                            static_cast<uint32_t>(imageBarriers.size()),
                            imageBarriers.data());

    m_ImageBarrierDescs.clear();
    m_MemoryBarrierDescs.clear();

    return *this;
}

BarrierBuilder& BarrierBuilder::AddMemoryBarrier(ThsvsAccessType prevAccess, ThsvsAccessType nextAccess)
{
    m_MemoryBarrierDescs.emplace_back(prevAccess, nextAccess);

    return *this;
}

BarrierBuilder& BarrierBuilder::AddImageLayoutTransition(const Image& image,
                                                         ThsvsAccessType prevAccess,
                                                         ThsvsAccessType nextAccess,
                                                         VkImageAspectFlags aspectMask)
{
    assert(!image.IsNull());

    m_ImageBarrierDescs.emplace_back(image.GetImage(), prevAccess, nextAccess, aspectMask);

    return *this;
}

BarrierBuilder& BarrierBuilder::AddImageLayoutTransition(VkImage image,
                                                         ThsvsAccessType prevAccess,
                                                         ThsvsAccessType nextAccess,
                                                         VkImageAspectFlags aspectMask)
{
    assert(image != nullptr);

    m_ImageBarrierDescs.emplace_back(image, prevAccess, nextAccess, aspectMask);

    return *this;
}

} // namespace Grace
