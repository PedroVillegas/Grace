#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <Grace/Enums.hpp>
#include <Private/Grace/InternalContainers.hpp>

namespace Grace
{

class Image;
class Buffer;

/// Global barriers define a set of accesses on multiple resources at once.
/// If a buffer or image doesn't require a queue ownership transfer, or an image
/// doesn't require a layout transition (e.g. you're using one of the GENERAL
/// layouts) then a global barrier should be preferred.
/// Simply define the previous and next access types of resources affected.
struct MemoryBarrier
{
    std::vector<AccessType> accessesBefore;
    std::vector<AccessType> accessesAfter;
};

/// Buffer barriers should only be used for queue family ownership transfers
/// - otherwise, prefer global memory barriers
///
/// Access types are defined in the same way as for a global memory barrier, but
/// they only affect the buffer range identified by buffer, offset and size,
/// rather than all resources.
///
/// srcQueueFamilyIndex and dstQueueFamilyIndex will be passed unmodified into a
/// VkBufferMemoryBarrier.
///
/// A buffer barrier defining a queue ownership transfer needs to be executed
/// twice - once by a queue in the source queue family, and then once again by a
/// queue in the destination queue family, with a semaphore guaranteeing
/// execution order between them.
struct BufferBarrier
{
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize size;
    std::vector<AccessType> accessesBefore;
    std::vector<AccessType> accessesAfter;
    uint32_t srcQueueFamilyIndex;
    uint32_t dstQueueFamilyIndex;
};

/// Image barriers should only be used for queue family ownership transfers
/// or image layout transitions - otherwise, prefer global memory barriers
///
/// In general, it is better to use image barriers with THSVS_IMAGE_LAYOUT_OPTIMAL
/// than it is to use global barriers with images using either of the
/// THSVS_IMAGE_LAYOUT_GENERAL* layouts.
///
/// Access types are defined in the same way as for a global memory barrier, but
/// they only affect the image subresource range identified by image and
/// subresourceRange, rather than all resources.
///
/// srcQueueFamilyIndex, dstQueueFamilyIndex, image, and subresourceRange will
/// be passed unmodified into a VkImageMemoryBarrier.
///
/// An image barrier defining a queue ownership transfer needs to be executed
/// twice - once by a queue in the source queue family, and then once again by a
/// queue in the destination queue family, with a semaphore guaranteeing
/// execution order between them.
///
/// If discardContents is set to true, the contents of the image become
/// undefined after the barrier is executed, which can result in a performance
/// boost over attempting to preserve the contents.
/// This is particularly useful for transient images where the contents are
/// going to be immediately overwritten. A good example of when to use this is
/// when an application re-uses a presented image after vkAcquireNextImageKHR.
struct ImageBarrier
{
    VkImage image;
    VkImageSubresourceRange subresourceRange;
    std::vector<AccessType> accessesBefore;
    std::vector<AccessType> accessesAfter;
    ImageLayout prevLayout;
    ImageLayout nextLayout;
    VkBool32 discardContents;
    uint32_t srcQueueFamilyIndex;
    uint32_t dstQueueFamilyIndex;
};

class BarrierBuilder
{
public:
    BarrierBuilder& PipelineBarrier(VkCommandBuffer cmd);

    BarrierBuilder& AddMemoryBarrier(std::vector<AccessType>&& accessesBefore, std::vector<AccessType>&& accessesAfter);

    BarrierBuilder& AddImageBarrier(const Image& image,
                                    std::vector<AccessType>&& accessesBefore,
                                    std::vector<AccessType>&& accessesAfter);

    BarrierBuilder& AddBufferBarrier(const Buffer& buffer,
                                     std::vector<AccessType>&& accessesBefore,
                                     std::vector<AccessType>&& accessesAfter);

private:
    /// Translates a Grace::MemoryBarrier into a VkMemoryBarrier2
    static void GetVulkanMemoryBarrier(const MemoryBarrier& barrier, VkMemoryBarrier2& vkBarrierOut) ;

    /// Translates a Grace::BufferBarrier into a VkBufferMemoryBarrier2
    static void GetVulkanBufferMemoryBarrier(const BufferBarrier& barrier, VkBufferMemoryBarrier2& vkBarrierOut) ;

    /// Translates a Grace::ImageBarrier into a VkImageMemoryBarrier2
    static void GetVulkanImageMemoryBarrier(const ImageBarrier& barrier, VkImageMemoryBarrier2& vkBarrierOut) ;

private:
    MemoryBarrier mMemoryBarrier = {};
    std::vector<ImageBarrier> mImageBarriers = {};
    std::vector<BufferBarrier> mBufferBarriers = {};
};

} // namespace Grace
