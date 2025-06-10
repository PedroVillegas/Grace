#pragma once

#include <string>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Grace
{

[[nodiscard]] VkDeviceAddress GetBufferDeviceAddress(VkDevice device, VkBuffer buffer);

/// Description used to create a Buffer object
struct BufferDesc
{
    /// Name used to identify the buffer, e.g. in validation errors
    std::string name;
    /// Total allocation size in Bytes
    size_t allocSize;
    /// Specifies how the buffer is allowed to be used
    VkBufferUsageFlags usage;
    /// Flags used by VMA to optimize buffer allocation
    VmaAllocationCreateFlags allocFlags;
};

class Buffer
{
public:
    Buffer() = default;
    ~Buffer() = default;

    void Create(VkDevice device, VmaAllocator allocator, const BufferDesc& desc);

    void Cleanup(VmaAllocator allocator);

    /// @returns `true` if associated `VkBuffer` or `VmaAllocation` are null.
    [[nodiscard]] bool IsNull() const;

    /// @returns `VkBuffer` which holds the actual data of the buffer.
    [[nodiscard]] VkBuffer GetBuffer() const;

    /// @returns Pointer to the memory location of the buffer on the device.
    [[nodiscard]] VkDeviceAddress GetBDA() const;

    /// @returns `VmaAllocation` which represents a single memory allocation.
    [[nodiscard]] VmaAllocation GetAllocation() const;

    /// @returns `VmaAllocationInfo` which stores metadata of the memory allocation e.g allocation size.
    [[nodiscard]] VmaAllocationInfo2 GetAllocationInfo(VmaAllocator allocator) const;

private:
    VkBuffer m_Buffer = {};
    VmaAllocation m_Allocation = {};
    VkDeviceAddress m_DeviceAddress = {};
};

} // namespace Grace
