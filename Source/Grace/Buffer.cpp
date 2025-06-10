#include "Buffer.hpp"

#include <Grace/Context.hpp>
#include <Grace/DebugReporter.hpp>

#include <cassert>

namespace Grace
{

bool Buffer::IsNull() const
{
    return (m_Buffer == nullptr || m_Allocation == nullptr);
}

VkBuffer Buffer::GetBuffer() const
{
    return m_Buffer;
}

VkDeviceAddress Buffer::GetBDA() const
{
    assert(m_DeviceAddress != 0);
    return m_DeviceAddress;
}

VmaAllocation Buffer::GetAllocation() const
{
    return m_Allocation;
}

VmaAllocationInfo2 Buffer::GetAllocationInfo(VmaAllocator allocator) const
{
    assert(allocator != nullptr);

    VmaAllocationInfo2 info = {};
    vmaGetAllocationInfo2(allocator, m_Allocation, &info);
    return info;
}

void Buffer::Create(VkDevice device, VmaAllocator allocator, const BufferDesc& desc)
{
    assert(device != nullptr);
    assert(allocator != nullptr);

    // Allocate buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.size = desc.allocSize;
    bufferInfo.usage = desc.usage;

    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaAllocInfo.flags = desc.allocFlags;

    DebugReporter::Check(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &m_Buffer, &m_Allocation, nullptr));

    if (desc.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        m_DeviceAddress = GetBufferDeviceAddress(device, m_Buffer);
    }

    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
    nameInfo.objectHandle = (uint64_t) m_Buffer;
    nameInfo.pObjectName = desc.name.c_str();
    VK_SET_DEBUG_NAME(device, &nameInfo);
}

void Buffer::Cleanup(VmaAllocator allocator)
{
    assert(allocator != nullptr);
    vmaDestroyBuffer(allocator, m_Buffer, m_Allocation);
    m_Buffer = {};
    m_Allocation = {};
}

VkDeviceAddress GetBufferDeviceAddress(VkDevice device, VkBuffer buffer)
{
    assert(device != nullptr);
    assert(buffer != nullptr);

    // Find the address of the buffer
    VkBufferDeviceAddressInfo deviceAddressInfo = {};
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.buffer = buffer;

    VkDeviceAddress bufferAddress = vkGetBufferDeviceAddress(device, &deviceAddressInfo);

    return bufferAddress;
}

} // namespace Grace
