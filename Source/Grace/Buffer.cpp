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

VkBuffer Buffer::GetVkHandle() const
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

VmaAllocationInfo2 Buffer::GetAllocationInfo() const
{
    VmaAllocationInfo2 info = {};
    vmaGetAllocationInfo2(m_Device->GetVmaHandle(), m_Allocation, &info);
    return info;
}

Buffer::Buffer(Device* pDevice, const BufferDesc& desc) : m_Device(pDevice)
{
    assert(!m_Device->IsNull());

    // Allocate buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.size = desc.allocSize;
    bufferInfo.usage = desc.usage;

    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaAllocInfo.flags = desc.allocFlags;

    DebugReporter::Check(
        vmaCreateBuffer(m_Device->GetVmaHandle(), &bufferInfo, &vmaAllocInfo, &m_Buffer, &m_Allocation, nullptr));

    if (desc.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkBufferDeviceAddressInfo deviceAddressInfo = {};
        deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        deviceAddressInfo.pNext = nullptr;
        deviceAddressInfo.buffer = m_Buffer;

        m_DeviceAddress = vkGetBufferDeviceAddress(m_Device->GetVkHandle(), &deviceAddressInfo);
    }

    if (m_Buffer != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = (uint64_t) m_Buffer;
        nameInfo.pObjectName = desc.name.c_str();
        VK_SET_DEBUG_NAME(m_Device->GetVkHandle(), &nameInfo);
    }
}

Buffer::~Buffer()
{
    if (m_Buffer != nullptr)
    {
        vmaDestroyBuffer(m_Device->GetVmaHandle(), m_Buffer, m_Allocation);
    }
}

Buffer::Buffer(Buffer&& other) noexcept
    : m_Device(other.m_Device), m_Buffer(other.m_Buffer), m_Allocation(other.m_Allocation),
      m_DeviceAddress(other.m_DeviceAddress)
{
    other.m_Buffer = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    vmaDestroyBuffer(m_Device->GetVmaHandle(), m_Buffer, nullptr);

    m_Device = other.m_Device;
    m_Buffer = other.m_Buffer;
    m_Allocation = other.m_Allocation;
    m_DeviceAddress = other.m_DeviceAddress;
    other.m_Buffer = nullptr;

    return *this;
}

} // namespace Grace
