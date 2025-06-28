#include "Image.hpp"

#include <Grace/DebugReporter.hpp>
#include <Grace/Context.hpp>

#include <cassert>
#include <cmath>

namespace Grace
{

ImageView::~ImageView()
{
    if (m_View != nullptr && m_ParentImage != nullptr)
    {
        vkDestroyImageView(m_Device->GetVkHandle(), m_View, nullptr);
    }
}

ImageView::ImageView(Device* pDevice, const ImageViewDesc& desc) : m_Device(pDevice)
{
    assert(!m_Device->IsNull());

    m_ParentImage = desc.image;

    const VkImageAspectFlags aspectMask =
        (m_ParentImage->GetFormat() == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image = m_ParentImage->GetImage();
    info.format = m_ParentImage->GetFormat();
    info.subresourceRange.baseMipLevel = desc.mipLevel;
    info.subresourceRange.levelCount = desc.levelCount;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspectMask;
    info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    DebugReporter::Check(vkCreateImageView(m_Device->GetVkHandle(), &info, nullptr, &m_View));

    if (m_View != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        nameInfo.objectHandle = (uint64_t) m_View;
        nameInfo.pObjectName = desc.name.c_str();
        VK_SET_DEBUG_NAME(m_Device->GetVkHandle(), &nameInfo);
    }
}

ImageView::ImageView(ImageView&& other) noexcept
    : m_Device(other.m_Device), m_ParentImage(other.m_ParentImage), m_View(other.m_View)
{
    other.m_View = nullptr;
}

ImageView& ImageView::operator=(ImageView&& other) noexcept
{
    m_Device = other.m_Device;
    m_ParentImage = other.m_ParentImage;
    m_View = other.m_View;
    other.m_View = nullptr;

    return *this;
}

bool ImageView::IsNull() const
{
    return m_View == nullptr;
}

VkImageView ImageView::GetVkHandle() const
{
    return m_View;
}

void ImageView::MakeNull()
{
    m_View = nullptr;
}

uint32_t ImageView::GetStorageImgId() const
{
    return m_StorageImgId;
}

void ImageView::SetStorageImgId(uint32_t storageImgId)
{
    m_StorageImgId = storageImgId;
}

VkImageUsageFlags ImageView::GetUsageFlags() const
{
    return m_ParentImage->GetUsageFlags();
}

Image::~Image()
{
    if (!m_IsSwapchainImage)
    {
        vmaDestroyImage(m_Device->GetVmaHandle(), m_Image, m_Allocation);
    }
}

Image::Image(Device* pDevice, const ImageDesc& desc)
    : m_Device(pDevice), m_Format(desc.format), m_Extent(desc.dimensions), m_UsageFlags(desc.usage)
{
    assert(!pDevice->IsNull());
    assert(desc.usage != 0);

    const uint32_t mipLevels = desc.mipmapped ? GetMaxMipLevels() : 1;

    VkImageCreateInfo imgInfo = ImageCreateInfo(desc.usage);
    imgInfo.mipLevels = mipLevels;

    // For the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // Allocate and create the image
    DebugReporter::Check(
        vmaCreateImage(m_Device->GetVmaHandle(), &imgInfo, &allocInfo, &m_Image, &m_Allocation, nullptr));

    m_DefaultView = ImageView(m_Device,
                              {
                                  .name = desc.name,
                                  .image = this,
                                  .mipLevel = 0,
                                  .levelCount = mipLevels,
                              });

    if (m_Image != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = (uint64_t) m_Image;
        nameInfo.pObjectName = desc.name.c_str();
        VK_SET_DEBUG_NAME(m_Device->GetVkHandle(), &nameInfo);
    }
}

Image::Image(Device* pDevice, VkImage image, const ImageDesc& desc)
    : m_Device(pDevice), m_Image(image), m_Format(desc.format), m_Extent(desc.dimensions), m_UsageFlags(desc.usage),
      m_IsSwapchainImage(true)
{
    assert(!pDevice->IsNull());
    assert(image != nullptr);
    assert(desc.usage != 0);

    const uint32_t mipLevels = desc.mipmapped ? GetMaxMipLevels() : 1;

    VkImageCreateInfo imgInfo = ImageCreateInfo(desc.usage);
    imgInfo.mipLevels = mipLevels;

    m_DefaultView = ImageView(pDevice,
                              {
                                  .name = desc.name,
                                  .image = this,
                                  .mipLevel = 0,
                                  .levelCount = 1,
                              });

    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = (uint64_t) m_Image;
        nameInfo.pObjectName = desc.name.c_str();
        VK_SET_DEBUG_NAME(pDevice->GetVkHandle(), &nameInfo);
    }
}

Image::Image(Image&& other) noexcept
    : m_Device(other.m_Device), m_DefaultView(std::move(other.m_DefaultView)), m_Image(other.m_Image),
      m_Allocation(other.m_Allocation), m_Extent(other.m_Extent), m_Format(other.m_Format),
      m_UsageFlags(other.m_UsageFlags), m_IsSwapchainImage(other.m_IsSwapchainImage)
{
    other.m_Image = nullptr;
}

Image& Image::operator=(Image&& other) noexcept
{
    if (m_Device != nullptr)
    {
        vmaDestroyImage(m_Device->GetVmaHandle(), m_Image, nullptr);
    }

    m_Device = other.m_Device;
    m_Image = other.m_Image;
    m_Allocation = other.m_Allocation;
    m_Format = other.m_Format;
    m_Extent = other.m_Extent;
    m_UsageFlags = other.m_UsageFlags;
    m_IsSwapchainImage = other.m_IsSwapchainImage;
    m_DefaultView = std::move(other.m_DefaultView);
    other.m_Image = nullptr;

    return *this;
}

VkImageCreateInfo Image::ImageCreateInfo(VkImageUsageFlags usageFlags) const
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

void Image::SetStorageImgId(uint32_t id)
{
    m_StorageImgId = id;
}

void Image::SetSampledImgId(uint32_t id)
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
    const bool needsAllocationCheck = m_IsSwapchainImage ? false : m_Allocation == nullptr;
    return m_Image == nullptr || m_DefaultView.GetVkHandle() == nullptr || needsAllocationCheck || m_UsageFlags == 0;
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

uint32_t Image::GetMaxMipLevels() const
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(m_Extent.width, m_Extent.height)))) + 1;
}

VkImageUsageFlags Image::GetUsageFlags() const
{
    return m_UsageFlags;
}

VmaAllocation Image::GetAllocation() const
{
    return m_Allocation;
}

VmaAllocationInfo2 Image::GetAllocationInfo() const
{
    VmaAllocationInfo2 info = {};
    vmaGetAllocationInfo2(m_Device->GetVmaHandle(), m_Allocation, &info);
    return info;
}

} // namespace Grace
