#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <Grace/GraceApi.hpp>
#include <Grace/Macros.hpp>
#include <Grace/Enums.hpp>
#include <Grace/TypesVector.hpp>
#include <Grace/GpuResourceTraits.hpp>

namespace Grace
{

class Device;
class Image;

struct GRACE_API ImageViewDesc
{
    const char* name;
    Image* image;
    uint32_t mipLevel;
    uint32_t levelCount;
};

class GRACE_API ImageView
{
public:
    ~ImageView();
    ImageView() = default;
    ImageView(Device* pDevice, const ImageViewDesc& desc);

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkImageView handle more than once
    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;

    ImageView(ImageView&& other) noexcept;
    ImageView& operator=(ImageView&& other) noexcept;

    GRACE_NODISCARD bool IsNull() const;

    GRACE_NODISCARD VkImageView VkHandle() const;

    void MakeNull();

    GRACE_NODISCARD uint32_t GetStorageImgId() const;

    void SetStorageImgId(uint32_t storageImgId);

    GRACE_NODISCARD constexpr ImageUsage GetUsageFlags() const;

    GRACE_NODISCARD constexpr bool HasUsage(ImageUsage usage) const;

private:
    Device* mDevice = nullptr;
    Image* mParentImage = nullptr;
    VkImageView mView = nullptr;
    uint32_t mStorageImgId = 0;
};

class GRACE_API Image : public GpuBindlessCompatibleTag
{
public:
    ~Image();
    Image() = default;
    Image(Device* pDevice, const GpuImageDesc& desc);
    Image(Device* pDevice, VkImage image, const GpuImageDesc& desc); // Specifically for swapchain images

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkImage handle more than once
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    /// Sets index to resource in bindless array of Storage Images for access on GPU.
    void SetStorageImgId(uint32_t id);

    /// Sets index to resource in bindless array of Sampled Images for access on GPU.
    void SetSampledImgId(uint32_t id);

    /// @returns Index to resource in bindless array of Storage Images for access on GPU.
    GRACE_NODISCARD uint32_t GetStorageImgId() const;

    /// @returns Index to resource in bindless array of Sampled Images for access on GPU.
    GRACE_NODISCARD uint32_t GetSampledImgId() const;

    /// @returns `true` if associated `VkImage`, `VkImageView` or `VmaAllocation` are null.
    GRACE_NODISCARD bool Exists() const;

    /// @returns `VkImage` of image which holds actual data.
    GRACE_NODISCARD const VkImage& VkHandle() const;

    /// @returns `VkImageView` of image which tells you how the data is stored.
    GRACE_NODISCARD const ImageView& GetDefaultView() const;

    /// @returns Format per pixel of image.
    GRACE_NODISCARD const Format& GetFormat() const;

    /// @returns VkExtent2D of image.
    GRACE_NODISCARD UInt2 GetExtent2D() const;

    /// @returns VkExtent3D of image.
    GRACE_NODISCARD const UInt3& GetExtent3D() const;

    /// @returns Width of image.
    GRACE_NODISCARD uint32_t GetWidth() const;

    /// @returns Height of image.
    GRACE_NODISCARD uint32_t GetHeight() const;

    /// @returns Depth of image.
    GRACE_NODISCARD uint32_t GetDepth() const;

    GRACE_NODISCARD uint32_t GetMaxMipLevels() const;

    /// @returns Usage flags used to create image.
    GRACE_NODISCARD constexpr ImageUsage GetUsageFlags() const;

    GRACE_NODISCARD constexpr bool HasUsage(ImageUsage usage) const;

    /// @returns `VmaAllocation` which represents a single memory allocation.
    GRACE_NODISCARD const VmaAllocation& GetAllocation() const;

    /// @returns `VmaAllocationInfo` which stores metadata of the memory allocation e.g. allocation size.
    GRACE_NODISCARD VmaAllocationInfo2 GetAllocationInfo() const;

    GRACE_NODISCARD constexpr ImageAspect InferAspect() const;

private:
    Device* mDevice = nullptr;
    ImageView mDefaultView = {};
    VkImage mImage = nullptr;
    VmaAllocation mAllocation = nullptr;
    UInt3 mExtent = {};
    Format mFormat = {};
    ImageUsage mUsageFlags = {};

    // For bindless
    uint32_t mStorageImgId = 0;
    uint32_t mSampledImgId = 0;

    bool mIsSwapchainImage = false;
};

} // namespace Grace
