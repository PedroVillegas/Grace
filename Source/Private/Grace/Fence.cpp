#include <Grace/Fence.hpp>

#include <Grace/Device.hpp>
#include <Grace/HelperFunctions.hpp>

namespace Grace
{
Fence::~Fence()
{
    if (mFence != VK_NULL_HANDLE)
    {
        vkDestroyFence(mDevicePtr->VkHandle(), mFence, nullptr);
    }
}

Fence::Fence(Device* pDevice, const GpuFenceDesc& desc) : mDevicePtr(pDevice)
{
    VkFenceCreateInfo cinfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = static_cast<VkFenceCreateFlags>(desc.flags),
    };

    DebugReporter::Check(vkCreateFence(mDevicePtr->VkHandle(), &cinfo, nullptr, &mFence));
    AssignDebugName<VkFence>(mDevicePtr->VkHandle(), mFence, desc.name);
}

Fence::Fence(Fence&& other) noexcept : mDevicePtr(other.mDevicePtr), mFence(other.mFence)
{
    other.mDevicePtr = nullptr;
    other.mFence = VK_NULL_HANDLE;
}

Fence& Fence::operator=(Fence&& other) noexcept
{
    if (mFence != VK_NULL_HANDLE)
    {
        vkDestroyFence(mDevicePtr->VkHandle(), mFence, nullptr);
    }

    mDevicePtr = other.mDevicePtr;
    mFence = other.mFence;
    other.mFence = VK_NULL_HANDLE;

    return *this;
}

bool Fence::Exists() const
{
    return mFence == VK_NULL_HANDLE;
}

const VkFence& Fence::VkHandle() const
{
    return mFence;
}
} // namespace Grace
