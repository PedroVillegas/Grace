#pragma once

#include <string>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Grace
{

class Image;

inline VkImageUsageFlags DefaultImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

/// Description used to create an Image object
struct ImageDesc
{
    /// Name used to identify the image, e.g. in validation errors
    std::string name;
    /// Specifies the image's dimensions
    VkExtent3D dimensions;
    /// Specifies the image's format
    VkFormat format;
    /// Specifies how the image is allowed to be used
    VkImageUsageFlags usage;
    /// Specifies whether mipmaps should be generated
    bool mipmapped;
};

struct ImageView
{
    const Image* image = nullptr;
    VkImageView view = {};
    VkImageUsageFlags usageFlags = {};
    uint32_t storageImgId = {};

    void Create(VkDevice device,
                const std::string& name,
                const Image& image,
                uint32_t mipLevel,
                uint32_t levelCount);
};

class Image
{
public:
    Image() = default;
    ~Image() = default;

    void Create(VkDevice device, VmaAllocator allocator, const ImageDesc& desc);

    void CreateForSwapchain(VkDevice device, VkImage img, const ImageDesc& desc);

    /// Free all resources used by this instance.
    void Cleanup(VkDevice device, VmaAllocator allocator);

    /// Sets index to resource in bindless array of Storage Images for access on GPU.
    void SetStorageImgId(const uint32_t id);

    /// Sets index to resource in bindless array of Sampled Images for access on GPU.
    void SetSampledImgId(const uint32_t id);

    /// @returns Index to resource in bindless array of Storage Images for access on GPU.
    [[nodiscard]] uint32_t GetStorageImgId() const;

    /// @returns Index to resource in bindless array of Sampled Images for access on GPU.
    [[nodiscard]] uint32_t GetSampledImgId() const;

    /// @returns `true` if associated `VkImage`, `VkImageView` or `VmaAllocation` are null.
    [[nodiscard]] bool IsNull() const;

    /// @returns `VkImage` of image which holds actual data.
    [[nodiscard]] VkImage GetImage() const;

    /// @returns `VkImageView` of image which tells you how the data is stored.
    [[nodiscard]] const ImageView& GetDefaultView() const;

    /// @returns Format per pixel of image.
    [[nodiscard]] VkFormat GetFormat() const;

    /// @returns Format per pixel of image as `const *`.
    [[nodiscard]] const VkFormat* GetFormatPtr() const;

    /// @returns VkExtent2D of image.
    [[nodiscard]] VkExtent2D GetExtent2D() const;

    /// @returns VkExtent3D of image.
    [[nodiscard]] VkExtent3D GetExtent3D() const;

    /// @returns Width of image.
    [[nodiscard]] uint32_t GetWidth() const;

    /// @returns Height of image.
    [[nodiscard]] uint32_t GetHeight() const;

    /// @returns Depth of image.
    [[nodiscard]] uint32_t GetDepth() const;

    /// @returns Usage flags used to create image.
    [[nodiscard]] VkImageUsageFlags GetUsageFlags() const;

    /// @returns `VmaAllocation` which represents a single memory allocation.
    [[nodiscard]] VmaAllocation GetAllocation() const;

    /// @returns `VmaAllocationInfo` which stores metadata of the memory allocation e.g allocation size.
    [[nodiscard]] VmaAllocationInfo2 GetAllocationInfo(VmaAllocator allocator) const;

    /// @returns Debug name of image.
    [[nodiscard]] const std::string& GetName() const;

private:
    [[nodiscard]] VkImageCreateInfo ImageCreateInfo(VkImageUsageFlags usageFlags);

    std::string m_Name = {};
    VkImage m_Image = {};
    ImageView m_DefaultView = {};
    VmaAllocation m_Allocation = {};
    VkExtent3D m_Extent = {};
    VkFormat m_Format = {};
    VkImageUsageFlags m_UsageFlags = {};

    // For bindless
    uint32_t m_StorageImgId = {};
    uint32_t m_SampledImgId = {};

    bool m_IsSwapchainImage = false;
};

[[nodiscard]] VkImageView CreateImageView(
    VkDevice device, const std::string& name, VkImage image, VkFormat format, uint32_t mipLevel, uint32_t levelCount);

[[nodiscard]] uint32_t GetMaxMipLevels(uint32_t width, uint32_t height);

} // namespace Grace
