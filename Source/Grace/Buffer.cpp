#include "Buffer.hpp"

#include <Grace/CommandGroup.hpp>
#include <Grace/Context.hpp>
#include <Grace/DebugReporter.hpp>

#include <cassert>

namespace Grace
{

bool Buffer::IsNull() const
{
    return (mBuffer == nullptr || mAllocation == nullptr);
}

VkBuffer Buffer::GetVkHandle() const
{
    return mBuffer;
}

VkDeviceAddress Buffer::GetBDA() const
{
    assert(mDeviceAddress != 0);
    return mDeviceAddress;
}

VmaAllocation Buffer::GetAllocation() const
{
    return mAllocation;
}

VmaAllocationInfo2 Buffer::GetAllocationInfo() const
{
    VmaAllocationInfo2 info = {};
    vmaGetAllocationInfo2(mDevice->GetVmaHandle(), mAllocation, &info);
    return info;
}

Buffer::Buffer(Device* pDevice, const BufferDesc& desc) : mDevice(pDevice)
{
    assert(!mDevice->IsNull());

    const VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = desc.size,
        .usage = static_cast<VkBufferUsageFlags>(desc.usage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    const VmaAllocationCreateInfo vmaAllocInfo = {
        .flags = desc.allocFlags,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = 0,
        .preferredFlags = 0,
        .memoryTypeBits = 0,
        .pool = nullptr,
        .pUserData = nullptr,
        .priority = 1.0F,
    };

    DebugReporter::Check(
        vmaCreateBuffer(mDevice->GetVmaHandle(), &bufferInfo, &vmaAllocInfo, &mBuffer, &mAllocation, nullptr));
    assert(mBuffer != nullptr);
    AssignDebugName<VkBuffer>(mDevice->GetVkHandle(), mBuffer, desc.name);

    if (EnumBitmaskHasBitSet(desc.usage, BufferUsage::DeviceAddress))
    {
        const VkBufferDeviceAddressInfo deviceAddressInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext = nullptr,
            .buffer = mBuffer,
        };

        mDeviceAddress = vkGetBufferDeviceAddress(mDevice->GetVkHandle(), &deviceAddressInfo);
    }

    if (desc.data != nullptr)
    {
        const VkBufferCreateInfo stagingBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = desc.size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
        };

        const VmaAllocationCreateInfo stagingBufferAllocationInfo = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = 0,
            .preferredFlags = 0,
            .memoryTypeBits = 0,
            .pool = nullptr,
            .pUserData = nullptr,
            .priority = 1.0F,
        };

        VmaAllocation stagingBufferAllocation = nullptr;
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        DebugReporter::Check(vmaCreateBuffer(mDevice->GetVmaHandle(),
                                             &stagingBufferInfo,
                                             &stagingBufferAllocationInfo,
                                             &stagingBuffer,
                                             &stagingBufferAllocation,
                                             nullptr));
        vmaCopyMemoryToAllocation(mDevice->GetVmaHandle(), desc.data, stagingBufferAllocation, 0, desc.size);

        const CommandBuffer& cmd = mDevice->BeginSingleTimeCommands();
        const std::string debugLabel = desc.name + std::string(" | Data upload/Mip Gen");
        cmd.BeginDebugLabel(debugLabel.c_str(), { 1.0F, 1.0F, 1.0F, 1.0F });

        // Copy indices data to staging buffer, then copy staging buffer to index buffer
        const VkBufferCopy2 copyRegion = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
            .pNext = nullptr,
            .srcOffset = 0,
            .dstOffset = 0,
            .size = desc.size,
        };

        const VkCopyBufferInfo2 copyBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
            .pNext = nullptr,
            .srcBuffer = stagingBuffer,
            .dstBuffer = mBuffer,
            .regionCount = 1,
            .pRegions = &copyRegion,
        };

        vkCmdCopyBuffer2(cmd.GetVkCommandBuffer(), &copyBufferInfo);

        cmd.EndDebugLabel();
        pDevice->EndAndSubmitSingleTimeCommands();
        vmaDestroyBuffer(mDevice->GetVmaHandle(), stagingBuffer, stagingBufferAllocation);
    }
}

Buffer::~Buffer()
{
    if (mDevice != nullptr)
    {
        vmaDestroyBuffer(mDevice->GetVmaHandle(), mBuffer, mAllocation);
    }
}

Buffer::Buffer(Buffer&& other) noexcept
    : mDevice(other.mDevice), mBuffer(other.mBuffer), mAllocation(other.mAllocation),
      mDeviceAddress(other.mDeviceAddress)
{
    other.mDevice = VK_NULL_HANDLE;
    other.mBuffer = VK_NULL_HANDLE;
    other.mAllocation = VK_NULL_HANDLE;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (mDevice != nullptr)
    {
        vmaDestroyBuffer(mDevice->GetVmaHandle(), mBuffer, mAllocation);
    }

    mDevice = other.mDevice;
    mBuffer = other.mBuffer;
    mAllocation = other.mAllocation;
    mDeviceAddress = other.mDeviceAddress;
    other.mDevice = VK_NULL_HANDLE;
    other.mBuffer = VK_NULL_HANDLE;
    other.mAllocation = VK_NULL_HANDLE;

    return *this;
}

} // namespace Grace
