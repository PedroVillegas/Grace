#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <Grace/GraceApi.hpp>
#include <Grace/Macros.hpp>
#include <Grace/Enums.hpp>
#include <Grace/GpuResourceDescriptions.hpp>
#include <Grace/GpuResourceTraits.hpp>

namespace Grace
{

class Device;

class GRACE_API Buffer
{
public:
    ~Buffer();
    Buffer() = default;
    Buffer(Device* pDevice, const GpuBufferDesc& desc);

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkBuffer handle more than once
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    /// @returns `true` if associated `VkBuffer` or `VmaAllocation` are null.
    GRACE_NODISCARD bool Exists() const;

    /// @returns `VkBuffer` which holds the actual data of the buffer.
    GRACE_NODISCARD VkBuffer VkHandle() const;

    /// @returns Pointer to the memory location of the buffer on the device.
    GRACE_NODISCARD uint64_t GetBDA() const;

    /// @returns `VmaAllocation` which represents a single memory allocation.
    GRACE_NODISCARD VmaAllocation GetAllocation() const;

    /// @returns `VmaAllocationInfo` which stores metadata of the memory allocation e.g. allocation size.
    GRACE_NODISCARD VmaAllocationInfo2 GetAllocationInfo() const;

private:
    Device* mDevice = nullptr;
    VkBuffer mBuffer = nullptr;
    VmaAllocation mAllocation = nullptr;
    uint64_t mDeviceAddress = 0;
};

} // namespace Grace
