#include <Grace/GpuImage.hpp>
#include <Grace/CommandGroup.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/Context.hpp>
#include <Grace/Detail/InternalContainers.hpp>
#include <Grace/Detail/Assert.hpp>
#include <Grace/GpuBuffer.hpp>

#include <cmath>

namespace Grace
{

ImageView::~ImageView()
{
    if (mDevice != nullptr)
    {
        vkDestroyImageView(mDevice->VkHandle(), mView, nullptr);
    }
}

ImageView::ImageView(Device* pDevice, const ImageViewDesc& desc) : mDevice(pDevice)
{
    GRACE_ASSERT(!mDevice->IsNull());

    mParentImage = desc.image;

    const ImageAspect aspectMask = mParentImage->InferAspect();

    VkImageViewCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = mParentImage->VkHandle(),
        .viewType = VK_IMAGE_VIEW_TYPE_1D,
        .format = static_cast<VkFormat>(mParentImage->GetFormat()),
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        .subresourceRange = {
            .aspectMask = static_cast<VkImageAspectFlags>(aspectMask),
            .baseMipLevel = desc.mipLevel,
            .levelCount = desc.levelCount,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    info.viewType = mParentImage->GetExtent3D().y > 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_1D;
    info.viewType = mParentImage->GetExtent3D().z > 1 ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D;

    DebugReporter::Check(vkCreateImageView(mDevice->VkHandle(), &info, nullptr, &mView));
    AssignDebugName<VkImageView>(mDevice->VkHandle(), mView, desc.name);
}

ImageView::ImageView(ImageView&& other) noexcept
    : mDevice(other.mDevice), mParentImage(other.mParentImage), mView(other.mView), mStorageImgId(other.mStorageImgId)
{
    other.mView = nullptr;
}

ImageView& ImageView::operator=(ImageView&& other) noexcept
{
    if (mDevice != nullptr)
    {
        vkDestroyImageView(mDevice->VkHandle(), mView, nullptr);
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

VkImageView ImageView::VkHandle() const
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

Image::Image(Device* pDevice, const GpuImageDesc& desc)
    : mDevice(pDevice), mFormat(desc.format), mExtent(desc.dimensions), mUsageFlags(desc.usage)
{
    GRACE_ASSERT(!pDevice->IsNull());
    GRACE_ASSERT(desc.dimensions.x > 0);

    const uint32_t mipLevels = desc.mipmapped ? GetMaxMipLevels() : 1;
    const ImageAspect aspectMask = InferAspect();

    VkImageCreateInfo imgcinfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_1D,
        .format = static_cast<VkFormat>(mFormat),
        .extent = VkExtent3D(mExtent.x, mExtent.y, mExtent.z),
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = static_cast<VkImageUsageFlags>(desc.usage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    imgcinfo.imageType = desc.dimensions.y > 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D;
    imgcinfo.imageType = desc.dimensions.z > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;

    if (desc.mipmapped)
    {
        imgcinfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    const VmaAllocationCreateInfo allocInfo = {
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .preferredFlags = 0,
        .memoryTypeBits = 0,
        .pool = nullptr,
        .pUserData = nullptr,
        .priority = 1.0F,
    };

    DebugReporter::Check(
        vmaCreateImage(mDevice->GetVmaHandle(), &imgcinfo, &allocInfo, &mImage, &mAllocation, nullptr));
    AssignDebugName<VkImage>(mDevice->VkHandle(), mImage, desc.name);

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
            stagingBuffer = mDevice->Create<Buffer>({
                .name = "Staging Buffer",
                .usage = BufferUsage::TransferSrc,
                .allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                .size = desc.size,
                .data = nullptr,
            });

            mDevice->CopyMemoryToHostVisibleBuffer(stagingBuffer, 0, desc.data, desc.size);

            const VkImageMemoryBarrier2  layoutTransition = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = mImage,
                .subresourceRange = {
                    .aspectMask = static_cast<VkImageAspectFlags>(aspectMask),
                    .baseMipLevel = 0,
                    .levelCount = mipLevels,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            const VkDependencyInfo layoutTransitionDepInfo = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .memoryBarrierCount = 0,
                .pMemoryBarriers = nullptr,
                .bufferMemoryBarrierCount = 0,
                .pBufferMemoryBarriers = nullptr,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &layoutTransition,
            };

            vkCmdPipelineBarrier2(cmd.GetVkCommandBuffer(), &layoutTransitionDepInfo);

            // Copy image data to staging buffer, then copy staging buffer to image; image stays gpu visible only
            const VkBufferImageCopy2 copyRegion = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
                .pNext = nullptr,
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {
                    .aspectMask = static_cast<VkImageAspectFlags>(aspectMask),
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                    },
                .imageOffset = { .x = 0, .y = 0, .z = 0 },
                .imageExtent = {
                    .width = desc.dimensions.x,
                    .height = desc.dimensions.y,
                    .depth = desc.dimensions.z,
                },
            };

            const VkCopyBufferToImageInfo2 copyInfo = {
                .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
                .pNext = nullptr,
                .srcBuffer = mDevice->Get<Buffer>(stagingBuffer).VkHandle(),
                .dstImage = mImage,
                .dstImageLayout = VK_IMAGE_LAYOUT_GENERAL,
                .regionCount = 1,
                .pRegions = &copyRegion,
            };

            vkCmdCopyBufferToImage2(cmd.GetVkCommandBuffer(), &copyInfo);

            if (desc.mipmapped)
            {
                const VkMemoryBarrier2 memBarrier = {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                    .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
                    .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
                };

                const VkDependencyInfo memBarrierDepInfo = {
                    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .pNext = nullptr,
                    .dependencyFlags = 0,
                    .memoryBarrierCount = 1,
                    .pMemoryBarriers = &memBarrier,
                    .bufferMemoryBarrierCount = 0,
                    .pBufferMemoryBarriers = nullptr,
                    .imageMemoryBarrierCount = 0,
                    .pImageMemoryBarriers = nullptr,
                };

                vkCmdPipelineBarrier2(cmd.GetVkCommandBuffer(), &memBarrierDepInfo);

                VkExtent2D imageSize = VkExtent2D(desc.dimensions.x, desc.dimensions.y);
                for (uint32_t mip = 0; mip < mipLevels; mip++)
                {
                    VkExtent2D halfSize = imageSize;
                    halfSize.width /= 2;
                    halfSize.height /= 2;

                    if (mip < mipLevels - 1)
                    {
                        const VkImageBlit2  blitRegion = {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                            .pNext = nullptr,
                            .srcSubresource = {
                                .aspectMask = static_cast<VkImageAspectFlags>(aspectMask),
                                .mipLevel = mip,
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                            },
                            .srcOffsets = {
                                VkOffset3D(0, 0, 0),
                                VkOffset3D(imageSize.width, imageSize.height, 1),
                            },
                            .dstSubresource = {
                                .aspectMask = static_cast<VkImageAspectFlags>(aspectMask),
                                .mipLevel = mip + 1,
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                            },
                            .dstOffsets = {
                                VkOffset3D(0, 0, 0),
                                VkOffset3D(halfSize.width, halfSize.height, 1),
                            },
                        };

                        const VkBlitImageInfo2 blitInfo = {
                            .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                            .pNext = nullptr,
                            .srcImage = mImage,
                            .srcImageLayout = VK_IMAGE_LAYOUT_GENERAL,
                            .dstImage = mImage,
                            .dstImageLayout = VK_IMAGE_LAYOUT_GENERAL,
                            .regionCount = 1,
                            .pRegions = &blitRegion,
                            .filter = VK_FILTER_LINEAR,
                        };

                        cmd.BlitImage(blitInfo);
                        imageSize = halfSize;
                    }

                    const VkMemoryBarrier2 memBarrierAfter = {
                        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                        .pNext = nullptr,
                        .srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
                        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                        .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
                        .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                    };

                    const VkDependencyInfo memBarrierAfterDepInfo = {
                        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                        .pNext = nullptr,
                        .dependencyFlags = 0,
                        .memoryBarrierCount = 1,
                        .pMemoryBarriers = &memBarrierAfter,
                        .bufferMemoryBarrierCount = 0,
                        .pBufferMemoryBarriers = nullptr,
                        .imageMemoryBarrierCount = 0,
                        .pImageMemoryBarriers = nullptr,
                    };

                    vkCmdPipelineBarrier2(cmd.GetVkCommandBuffer(), &memBarrierAfterDepInfo);
                }
            }
        }

        if (desc.access != AccessType::None)
        {
            const VkImageMemoryBarrier2  layoutTransition = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
                .dstAccessMask = VK_ACCESS_2_NONE,
                .oldLayout = desc.data == nullptr ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_GENERAL,
                .newLayout = AccessTypeMap[static_cast<uint32_t>(desc.access)].imageLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = mImage,
                .subresourceRange = {
                    .aspectMask = static_cast<VkImageAspectFlags>(aspectMask),
                    .baseMipLevel = 0,
                    .levelCount = mipLevels,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            const VkDependencyInfo depInfo = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .memoryBarrierCount = 0,
                .pMemoryBarriers = nullptr,
                .bufferMemoryBarrierCount = 0,
                .pBufferMemoryBarriers = nullptr,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &layoutTransition,
            };

            vkCmdPipelineBarrier2(cmd.GetVkCommandBuffer(), &depInfo);
        }

        cmd.EndDebugLabel();
        mDevice->EndAndSubmitSingleTimeCommands();

        if (desc.data != nullptr)
        {
            mDevice->Free<Buffer>(stagingBuffer);
        }
    }
}

Image::Image(Device* pDevice, VkImage image, const GpuImageDesc& desc)
    : mDevice(pDevice), mImage(image), mFormat(desc.format), mExtent(desc.dimensions), mUsageFlags(desc.usage),
      mIsSwapchainImage(true)
{
    GRACE_ASSERT(!pDevice->IsNull());
    GRACE_ASSERT(image != nullptr);

    mDefaultView = ImageView(pDevice,
                             {
                                 .name = desc.name,
                                 .image = this,
                                 .mipLevel = 0,
                                 .levelCount = 1,
                             });

    AssignDebugName<VkImage>(mDevice->VkHandle(), mImage, desc.name);
}

Image::Image(Image&& other) noexcept
    : mDevice(other.mDevice), mDefaultView(std::move(other.mDefaultView)), mImage(other.mImage),
      mAllocation(other.mAllocation), mExtent(other.mExtent), mFormat(other.mFormat), mUsageFlags(other.mUsageFlags),
      mStorageImgId(other.mStorageImgId), mSampledImgId(other.mSampledImgId), mIsSwapchainImage(other.mIsSwapchainImage)
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
    GRACE_ASSERT(HasUsage(ImageUsage::StorageImage));
    return mStorageImgId;
}

uint32_t Image::GetSampledImgId() const
{
    GRACE_ASSERT(HasUsage(ImageUsage::SampledImage));
    return mSampledImgId;
}

bool Image::Exists() const
{
    const bool needsAllocationCheck = mIsSwapchainImage ? false : mAllocation == nullptr;
    return mImage == nullptr || mDefaultView.VkHandle() == nullptr || needsAllocationCheck;
}

const VkImage& Image::VkHandle() const
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
