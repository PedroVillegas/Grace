#include "Image.hpp"

#include <Grace/DebugReporter.hpp>
#include <Grace/Context.hpp>

#include <cassert>
#include <cmath>

namespace Grace
{

void ImageView::Create(
    VkDevice device, const std::string& name, const Image& image, uint32_t mipLevel, uint32_t levelCount)
{
    assert(device != nullptr);

    usageFlags = image.GetUsageFlags();
    view = CreateImageView(device, name, image.GetImage(), image.GetFormat(), mipLevel, levelCount);
}

void Image::Create(VkDevice device, VmaAllocator allocator, const ImageDesc& desc)
{
    assert(device != nullptr);
    assert(allocator != nullptr);
    assert(desc.usage != 0);

    m_Name = desc.name;
    m_Format = desc.format;
    m_Extent = desc.dimensions;
    m_UsageFlags = desc.usage;

    uint32_t mipLevels = desc.mipmapped ? GetMaxMipLevels(desc.dimensions.width, desc.dimensions.height) : 1;

    VkImageCreateInfo imgInfo = ImageCreateInfo(desc.usage);
    imgInfo.mipLevels = mipLevels;

    // For the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Allocate and create the image
    DebugReporter::Check(vmaCreateImage(allocator, &imgInfo, &allocInfo, &m_Image, &m_Allocation, nullptr));

    // Generate default image view
    VkImageAspectFlags aspectMask =
        (m_Format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image = m_Image;
    info.format = m_Format;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = mipLevels;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspectMask;
    info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    VkImageView out;
    DebugReporter::Check(vkCreateImageView(device, &info, nullptr, &out));

    m_DefaultView = { .image = this, .view = out, .usageFlags = m_UsageFlags, .storageImgId = m_StorageImgId };

    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        nameInfo.objectHandle = (uint64_t) out;
        nameInfo.pObjectName = m_Name.c_str();
        VK_SET_DEBUG_NAME(device, &nameInfo);
    }

    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = (uint64_t) m_Image;
        nameInfo.pObjectName = desc.name.c_str();
        VK_SET_DEBUG_NAME(device, &nameInfo);
    }
}

void Image::CreateForSwapchain(VkDevice device, VkImage img, const ImageDesc& desc)
{
    assert(device != nullptr);
    assert(img != nullptr);
    assert(desc.usage != 0);

    m_Name = desc.name;
    m_Format = desc.format;
    m_Extent = desc.dimensions;
    m_UsageFlags = desc.usage;
    m_Image = img;
    m_IsSwapchainImage = true;

    uint32_t mipLevels = desc.mipmapped ? GetMaxMipLevels(desc.dimensions.width, desc.dimensions.height) : 1;

    VkImageCreateInfo imgInfo = ImageCreateInfo(desc.usage);
    imgInfo.mipLevels = mipLevels;

    // Generate default image view
    VkImageAspectFlags aspectMask =
        (m_Format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image = m_Image;
    info.format = m_Format;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = mipLevels;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspectMask;
    info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    VkImageView out;
    DebugReporter::Check(vkCreateImageView(device, &info, nullptr, &out));

    m_DefaultView = { .image = this, .view = out, .usageFlags = m_UsageFlags, .storageImgId = m_StorageImgId };

    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        nameInfo.objectHandle = (uint64_t) out;
        nameInfo.pObjectName = m_Name.c_str();
        VK_SET_DEBUG_NAME(device, &nameInfo);
    }

    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = (uint64_t) m_Image;
        nameInfo.pObjectName = desc.name.c_str();
        VK_SET_DEBUG_NAME(device, &nameInfo);
    }
}

void Image::Cleanup(VkDevice device, VmaAllocator allocator)
{
    assert(device != nullptr);
    assert(allocator != nullptr);

    vkDestroyImageView(device, m_DefaultView.view, nullptr);
    if (!m_IsSwapchainImage)
    {
        vmaDestroyImage(allocator, m_Image, m_Allocation);
    }
}

VkImageCreateInfo Image::ImageCreateInfo(VkImageUsageFlags usageFlags)
{
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = m_Format;
    info.extent = m_Extent;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;

    // Optimal tiling, which means the image is stored on the best gpu format
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usageFlags;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    return info;
}

VkImageView CreateImageView(
    VkDevice device, const std::string& name, VkImage image, VkFormat format, uint32_t mipLevel, uint32_t levelCount)
{
    assert(device != nullptr);
    assert(image != nullptr);

    VkImageAspectFlags aspectMask =
        (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image = image;
    info.format = format;
    info.subresourceRange.baseMipLevel = mipLevel;
    info.subresourceRange.levelCount = levelCount;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspectMask;

    VkImageView out;
    DebugReporter::Check(vkCreateImageView(device, &info, nullptr, &out));

    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        nameInfo.objectHandle = (uint64_t) out;
        nameInfo.pObjectName = name.c_str();
        VK_SET_DEBUG_NAME(device, &nameInfo);
    }

    return out;
}

uint32_t GetMaxMipLevels(uint32_t width, uint32_t height)
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

void Image::SetStorageImgId(const uint32_t id)
{
    m_StorageImgId = id;
}

void Image::SetSampledImgId(const uint32_t id)
{
    m_SampledImgId = id;
}

uint32_t Image::GetStorageImgId() const
{
    assert(m_UsageFlags & VK_IMAGE_USAGE_STORAGE_BIT);
    return m_StorageImgId;
}

uint32_t Image::GetSampledImgId() const
{
    assert(m_UsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT);
    return m_SampledImgId;
}

bool Image::IsNull() const
{
    bool needsAllocationCheck = m_IsSwapchainImage ? false : m_Allocation == nullptr;
    return m_Image == nullptr || m_DefaultView.view == nullptr || needsAllocationCheck || m_UsageFlags == 0;
}

VkImage Image::GetImage() const
{
    return m_Image;
}

const ImageView& Image::GetDefaultView() const
{
    return m_DefaultView;
}

VkFormat Image::GetFormat() const
{
    return m_Format;
}

const VkFormat* Image::GetFormatPtr() const
{
    return &m_Format;
}

VkExtent2D Image::GetExtent2D() const
{
    return { m_Extent.width, m_Extent.height };
}

VkExtent3D Image::GetExtent3D() const
{
    return m_Extent;
}

uint32_t Image::GetWidth() const
{
    return m_Extent.width;
}

uint32_t Image::GetHeight() const
{
    return m_Extent.height;
}

uint32_t Image::GetDepth() const
{
    return m_Extent.depth;
}

VkImageUsageFlags Image::GetUsageFlags() const
{
    return m_UsageFlags;
}

VmaAllocation Image::GetAllocation() const
{
    return m_Allocation;
}

VmaAllocationInfo2 Image::GetAllocationInfo(VmaAllocator allocator) const
{
    assert(allocator != nullptr);
    VmaAllocationInfo2 info = {};
    vmaGetAllocationInfo2(allocator, m_Allocation, &info);
    return info;
}

const std::string& Image::GetName() const
{
    return m_Name;
}

} // namespace Grace
