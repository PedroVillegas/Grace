#include "Fence.hpp"

#include <Grace/Device.hpp>
#include <Grace/HelperFunctions.hpp>

Grace::Fence::~Fence()
{
    if (mFence != VK_NULL_HANDLE)
    {
        vkDestroyFence(mDevicePtr->GetVkHandle(), mFence, nullptr);
    }
}

Grace::Fence::Fence(Device* pDevice, const FenceDesc& desc) : mDevicePtr(pDevice)
{
    VkFenceCreateInfo cinfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = static_cast<VkFenceCreateFlags>(desc.flags),
    };

    DebugReporter::Check(vkCreateFence(mDevicePtr->GetVkHandle(), &cinfo, nullptr, &mFence));
    AssignDebugName<VkFence>(mDevicePtr->GetVkHandle(), mFence, desc.name);
}

Grace::Fence::Fence(Fence&& other) noexcept : mDevicePtr(other.mDevicePtr), mFence(other.mFence)
{
    other.mDevicePtr = nullptr;
    other.mFence = VK_NULL_HANDLE;
}

Grace::Fence& Grace::Fence::operator=(Fence&& other) noexcept
{
    if (mFence != VK_NULL_HANDLE)
    {
        vkDestroyFence(mDevicePtr->GetVkHandle(), mFence, nullptr);
    }

    mDevicePtr = other.mDevicePtr;
    mFence = other.mFence;
    other.mFence = VK_NULL_HANDLE;

    return *this;
}

bool Grace::Fence::IsNull() const
{
    return mFence == VK_NULL_HANDLE;
}

const VkFence& Grace::Fence::GetVkFence() const
{
    return mFence;
}
