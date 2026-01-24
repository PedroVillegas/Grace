#include "Image.hpp"

#include <Grace/CommandGroup.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/Context.hpp>

#include <cassert>
#include <cmath>

namespace Grace
{

ImageView::~ImageView()
{
    if (mDevice != nullptr)
    {
        vkDestroyImageView(mDevice->GetVkHandle(), mView, nullptr);
    }
}

ImageView::ImageView(Device* pDevice, const ImageViewDesc& desc) : mDevice(pDevice)
{
    assert(!mDevice->IsNull());

    mParentImage = desc.image;

    const ImageAspect aspectMask = mParentImage->InferAspect();

    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;
    info.viewType = mParentImage->GetExtent3D().y > 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_1D;
    info.viewType = mParentImage->GetExtent3D().z > 1 ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D;
    info.image = mParentImage->GetImage();
    info.format = static_cast<VkFormat>(mParentImage->GetFormat());
    info.subresourceRange.baseMipLevel = desc.mipLevel;
    info.subresourceRange.levelCount = desc.levelCount;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = static_cast<VkImageAspectFlags>(aspectMask);
    info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    DebugReporter::Check(vkCreateImageView(mDevice->GetVkHandle(), &info, nullptr, &mView));
    AssignDebugName<VkImageView>(mDevice->GetVkHandle(), mView, desc.name);
}

ImageView::ImageView(ImageView&& other) noexcept
    : mDevice(other.mDevice), mParentImage(other.mParentImage), mView(other.mView),
      mStorageImgId(other.mStorageImgId)
{
    other.mView = nullptr;
}

ImageView& ImageView::operator=(ImageView&& other) noexcept
{
    if (mDevice != nullptr)
    {
        vkDestroyImageView(mDevice->GetVkHandle(), mView, nullptr);
    }

    mDevice = other.mDevice;
    mParentImage = other.mParentImage;
    mView = other.mView;
    mStorageImgId = other.mStorageImgId;
    other.mDevice = nullptr;
    other.mParentImage = nullptr;
    other.mView = nullptr;

    return *this;
}

bool ImageView::IsNull() const
{
    return mView == nullptr;
}

VkImageView ImageView::GetVkHandle() const
{
    return mView;
}

void ImageView::MakeNull()
{
    mView = nullptr;
}

uint32_t ImageView::GetStorageImgId() const
{
    return mStorageImgId;
}

void ImageView::SetStorageImgId(uint32_t storageImgId)
{
    mStorageImgId = storageImgId;
}

constexpr ImageUsage ImageView::GetUsageFlags() const
{
    return mParentImage->GetUsageFlags();
}

constexpr bool ImageView::HasUsage(ImageUsage usage) const
{
    return mParentImage->HasUsage(usage);
}

Image::~Image()
{
    if (mDevice != nullptr && !mIsSwapchainImage)
    {
        vmaDestroyImage(mDevice->GetVmaHandle(), mImage, mAllocation);
    }
}

Image::Image(Device* pDevice, const ImageDesc& desc)
    : mDevice(pDevice), mFormat(desc.format), mExtent(desc.dimensions), mUsageFlags(desc.usage)
{
    assert(!pDevice->IsNull());
    assert(desc.dimensions.x > 0);

    const uint32_t mipLevels = desc.mipmapped ? GetMaxMipLevels() : 1;
    const ImageAspect aspectMask = InferAspect();

    VkImageCreateInfo imgcinfo = {};
    imgcinfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgcinfo.pNext = nullptr;
    imgcinfo.flags = 0;
    imgcinfo.imageType = desc.dimensions.y > 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D;
    imgcinfo.imageType = desc.dimensions.z > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    imgcinfo.format = static_cast<VkFormat>(mFormat);
    imgcinfo.extent = VkExtent3D(mExtent.x, mExtent.y, mExtent.z);
    imgcinfo.mipLevels = mipLevels;
    imgcinfo.arrayLayers = 1;
    imgcinfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgcinfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgcinfo.usage = static_cast<VkImageUsageFlags>(desc.usage);
    if (desc.mipmapped)
        imgcinfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imgcinfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    DebugReporter::Check(
        vmaCreateImage(mDevice->GetVmaHandle(), &imgcinfo, &allocInfo, &mImage, &mAllocation, nullptr));
    AssignDebugName<VkImage>(mDevice->GetVkHandle(), mImage, desc.name);

    mDefaultView = ImageView(mDevice,
                              {
                                  .name = desc.name,
                                  .image = this,
                                  .mipLevel = 0,
                                  .levelCount = mipLevels,
                              });

    if (desc.data != nullptr || desc.access != AccessType::None)
    {
        BufferHandle stagingBuffer;

        const CommandBuffer& cmd = mDevice->BeginSingleTimeCommands();
        const std::string debugLabel = desc.name + std::string(" | Setup");
        cmd.BeginDebugLabel(debugLabel.c_str(), { 1.0F, 1.0F, 1.0F, 1.0F });

        if (desc.data != nullptr)
        {
            stagingBuffer = pDevice->CreateBuffer({
                .name = "Staging Buffer",
                .usage = BufferUsage::TransferSrc,
                .allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                .size = desc.size,
                .data = nullptr,
            });

            mDevice->CopyMemoryToHostVisibleBuffer(stagingBuffer, 0, desc.data, desc.size);

            VkImageMemoryBarrier2 layoutTransition = {};
            layoutTransition.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            layoutTransition.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
            layoutTransition.srcAccessMask = VK_ACCESS_2_NONE;
            layoutTransition.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
            layoutTransition.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            layoutTransition.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            layoutTransition.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            layoutTransition.image = mImage;
            layoutTransition.subresourceRange.aspectMask = static_cast<VkImageAspectFlags>(aspectMask);
            layoutTransition.subresourceRange.baseMipLevel = 0;
            layoutTransition.subresourceRange.levelCount = mipLevels;
            layoutTransition.subresourceRange.baseArrayLayer = 0;
            layoutTransition.subresourceRange.layerCount = 1;

            VkDependencyInfo depInfo = {};
            depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            depInfo.imageMemoryBarrierCount = 1;
            depInfo.pImageMemoryBarriers = &layoutTransition;

            vkCmdPipelineBarrier2(cmd.GetVkCommandBuffer(), &depInfo);

            // Copy image data to staging buffer, then copy staging buffer to image; image stays gpu visible only
            VkBufferImageCopy2 copyRegion = {};
            copyRegion.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
            copyRegion.pNext = nullptr;
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;
            copyRegion.imageSubresource.aspectMask = static_cast<VkImageAspectFlags>(aspectMask);
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageOffset = { .x = 0, .y = 0, .z = 0 };
            copyRegion.imageExtent = { .width = desc.dimensions.x,
                                       .height = desc.dimensions.y,
                                       .depth = desc.dimensions.z };

            VkCopyBufferToImageInfo2 copyInfo = {};
            copyInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
            copyInfo.pNext = nullptr;
            copyInfo.srcBuffer = mDevice->GetBuffer(stagingBuffer).GetVkHandle();
            copyInfo.dstImage = mImage;
            copyInfo.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
            copyInfo.regionCount = 1;
            copyInfo.pRegions = &copyRegion;

            vkCmdCopyBufferToImage2(cmd.GetVkCommandBuffer(), &copyInfo);

            if (desc.mipmapped)
            {
                {
                    VkMemoryBarrier2 memBarrier = {};
                    memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
                    memBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
                    memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                    memBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
                    memBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;

                    VkDependencyInfo depInfo = {};
                    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                    depInfo.memoryBarrierCount = 1;
                    depInfo.pMemoryBarriers = &memBarrier;

                    vkCmdPipelineBarrier2(cmd.GetVkCommandBuffer(), &depInfo);
                }

                VkExtent2D imageSize = { desc.dimensions.x, desc.dimensions.y };
                for (uint32_t mip = 0; mip < mipLevels; mip++)
                {
                    VkExtent2D halfSize = imageSize;
                    halfSize.width /= 2;
                    halfSize.height /= 2;

                    if (mip < mipLevels - 1)
                    {
                        VkImageBlit2 blitRegion = {};
                        blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
                        blitRegion.pNext = nullptr;
                        blitRegion.srcOffsets[1].x = imageSize.width;
                        blitRegion.srcOffsets[1].y = imageSize.height;
                        blitRegion.srcOffsets[1].z = 1;
                        blitRegion.dstOffsets[1].x = halfSize.width;
                        blitRegion.dstOffsets[1].y = halfSize.height;
                        blitRegion.dstOffsets[1].z = 1;
                        blitRegion.srcSubresource.aspectMask = static_cast<VkImageAspectFlags>(aspectMask);
                        blitRegion.srcSubresource.baseArrayLayer = 0;
                        blitRegion.srcSubresource.layerCount = 1;
                        blitRegion.srcSubresource.mipLevel = mip;
                        blitRegion.dstSubresource.aspectMask = static_cast<VkImageAspectFlags>(aspectMask);
                        blitRegion.dstSubresource.baseArrayLayer = 0;
                        blitRegion.dstSubresource.layerCount = 1;
                        blitRegion.dstSubresource.mipLevel = mip + 1;

                        VkBlitImageInfo2 blitInfo = {};
                        blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
                        blitInfo.pNext = nullptr;
                        blitInfo.srcImage = mImage;
                        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
                        blitInfo.dstImage = mImage;
                        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
                        blitInfo.filter = VK_FILTER_LINEAR;
                        blitInfo.regionCount = 1;
                        blitInfo.pRegions = &blitRegion;

                        cmd.BlitImage(blitInfo);
                        imageSize = halfSize;
                    }

                    {
                        VkMemoryBarrier2 memBarrier = {};
                        memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
                        memBarrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
                        memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        memBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
                        memBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

                        VkDependencyInfo depInfo = {};
                        depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                        depInfo.memoryBarrierCount = 1;
                        depInfo.pMemoryBarriers = &memBarrier;

                        vkCmdPipelineBarrier2(cmd.GetVkCommandBuffer(), &depInfo);
                    }
                }
            }
        }

        if (desc.access != AccessType::None)
        {
            VkImageMemoryBarrier2 layoutTransition = {};
            layoutTransition.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            layoutTransition.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
            layoutTransition.srcAccessMask = VK_ACCESS_2_NONE;
            layoutTransition.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
            layoutTransition.dstAccessMask = VK_ACCESS_2_NONE;
            layoutTransition.oldLayout = desc.data == nullptr ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_GENERAL;
            layoutTransition.newLayout = AccessTypeMap[static_cast<uint32_t>(desc.access)].imageLayout;
            layoutTransition.image = mImage;
            layoutTransition.subresourceRange.aspectMask = static_cast<VkImageAspectFlags>(aspectMask);
            layoutTransition.subresourceRange.baseMipLevel = 0;
            layoutTransition.subresourceRange.levelCount = mipLevels;
            layoutTransition.subresourceRange.baseArrayLayer = 0;
            layoutTransition.subresourceRange.layerCount = 1;

            VkDependencyInfo depInfo = {};
            depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            depInfo.imageMemoryBarrierCount = 1;
            depInfo.pImageMemoryBarriers = &layoutTransition;

            vkCmdPipelineBarrier2(cmd.GetVkCommandBuffer(), &depInfo);
        }

        cmd.EndDebugLabel();
        mDevice->EndAndSubmitSingleTimeCommands();

        if (desc.data != nullptr)
        {
            mDevice->FreeBuffer(stagingBuffer);
        }
    }
}

Image::Image(Device* pDevice, VkImage image, const ImageDesc& desc)
    : mDevice(pDevice), mImage(image), mFormat(desc.format), mExtent(desc.dimensions), mUsageFlags(desc.usage),
      mIsSwapchainImage(true)
{
    assert(!pDevice->IsNull());
    assert(image != nullptr);

    mDefaultView = ImageView(pDevice,
                              {
                                  .name = desc.name,
                                  .image = this,
                                  .mipLevel = 0,
                                  .levelCount = 1,
                              });

    AssignDebugName<VkImage>(mDevice->GetVkHandle(), mImage, desc.name);
}

Image::Image(Image&& other) noexcept
    : mDevice(other.mDevice), mDefaultView(std::move(other.mDefaultView)), mImage(other.mImage),
      mAllocation(other.mAllocation), mExtent(other.mExtent), mFormat(other.mFormat),
      mUsageFlags(other.mUsageFlags), mStorageImgId(other.mStorageImgId), mSampledImgId(other.mSampledImgId),
      mIsSwapchainImage(other.mIsSwapchainImage)
{
    other.mDevice = VK_NULL_HANDLE;
    other.mImage = VK_NULL_HANDLE;
    other.mAllocation = VK_NULL_HANDLE;
}

Image& Image::operator=(Image&& other) noexcept
{
    if (mDevice != nullptr && !mIsSwapchainImage)
    {
        vmaDestroyImage(mDevice->GetVmaHandle(), mImage, mAllocation);
    }

    mDevice = other.mDevice;
    mImage = other.mImage;
    mAllocation = other.mAllocation;
    mFormat = other.mFormat;
    mExtent = other.mExtent;
    mUsageFlags = other.mUsageFlags;
    mIsSwapchainImage = other.mIsSwapchainImage;
    mDefaultView = std::move(other.mDefaultView);
    mStorageImgId = other.mStorageImgId;
    mSampledImgId = other.mSampledImgId;
    other.mDevice = VK_NULL_HANDLE;
    other.mImage = VK_NULL_HANDLE;
    other.mAllocation = VK_NULL_HANDLE;

    return *this;
}

void Image::SetStorageImgId(uint32_t id)
{
    mStorageImgId = id;
}

void Image::SetSampledImgId(uint32_t id)
{
    mSampledImgId = id;
}

uint32_t Image::GetStorageImgId() const
{
    assert(HasUsage(ImageUsage::StorageImage));
    return mStorageImgId;
}

uint32_t Image::GetSampledImgId() const
{
    assert(HasUsage(ImageUsage::SampledImage));
    return mSampledImgId;
}

bool Image::IsNull() const
{
    const bool needsAllocationCheck = mIsSwapchainImage ? false : mAllocation == nullptr;
    return mImage == nullptr || mDefaultView.GetVkHandle() == nullptr || needsAllocationCheck;
}

const VkImage& Image::GetImage() const
{
    return mImage;
}

const ImageView& Image::GetDefaultView() const
{
    return mDefaultView;
}

const Format& Image::GetFormat() const
{
    return mFormat;
}

UInt2 Image::GetExtent2D() const
{
    return { mExtent.x, mExtent.y };
}

const UInt3& Image::GetExtent3D() const
{
    return mExtent;
}

uint32_t Image::GetWidth() const
{
    return mExtent.x;
}

uint32_t Image::GetHeight() const
{
    return mExtent.y;
}

uint32_t Image::GetDepth() const
{
    return mExtent.z;
}

uint32_t Image::GetMaxMipLevels() const
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(mExtent.x, mExtent.y)))) + 1;
}

constexpr ImageUsage Image::GetUsageFlags() const
{
    return mUsageFlags;
}

constexpr bool Image::HasUsage(ImageUsage usage) const
{
    return static_cast<bool>(mUsageFlags & usage);
}

const VmaAllocation& Image::GetAllocation() const
{
    return mAllocation;
}

VmaAllocationInfo2 Image::GetAllocationInfo() const
{
    VmaAllocationInfo2 info = {};
    vmaGetAllocationInfo2(mDevice->GetVmaHandle(), mAllocation, &info);
    return info;
}

constexpr ImageAspect Image::InferAspect() const
{
    switch (mFormat)
    {
    case Format::D32_SFloat:
    case Format::D16_UNorm:
        return ImageAspect::Depth;
    case Format::S8_UInt:
        return ImageAspect::Stencil;
    case Format::D16_UNorm_S8_UInt:
    case Format::D32_SFloat_S8_UInt:
    case Format::D24_UNorm_S8_UInt:
        return ImageAspect::Depth | ImageAspect::Stencil;
    default:
        return ImageAspect::Color;
    }
}

} // namespace Grace
