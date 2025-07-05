#pragma once

#include <string>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <Grace/GraceExport.h>
#include <Grace/Macros.hpp>

namespace Grace
{

class Device;

/// Description used to create a Buffer object
struct GRACE_EXPORT BufferDesc
{
    /// Name used to identify the buffer, e.g. in validation errors
    const char* name;
    /// Total allocation size in Bytes
    size_t allocSize;
    /// Specifies how the buffer is allowed to be used
    VkBufferUsageFlags usage;
    /// Flags used by VMA to optimize buffer allocation
    VmaAllocationCreateFlags allocFlags;
};

class GRACE_EXPORT Buffer
{
public:
    ~Buffer();
    Buffer() = default;
    Buffer(Device* pDevice, const BufferDesc& desc);

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkBuffer handle more than once
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    /// @returns `true` if associated `VkBuffer` or `VmaAllocation` are null.
    _NODISCARD bool IsNull() const;

    /// @returns `VkBuffer` which holds the actual data of the buffer.
    _NODISCARD VkBuffer GetVkHandle() const;

    /// @returns Pointer to the memory location of the buffer on the device.
    _NODISCARD uint64_t GetBDA() const;

    /// @returns `VmaAllocation` which represents a single memory allocation.
    _NODISCARD VmaAllocation GetAllocation() const;

    /// @returns `VmaAllocationInfo` which stores metadata of the memory allocation e.g. allocation size.
    _NODISCARD VmaAllocationInfo2 GetAllocationInfo() const;

private:
    Device* m_Device = nullptr;
    VkBuffer m_Buffer = nullptr;
    VmaAllocation m_Allocation = nullptr;
    uint64_t m_DeviceAddress = 0;
};

} // namespace Grace
