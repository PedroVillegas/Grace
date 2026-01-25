#include "Semaphore.hpp"

#include <Grace/Device.hpp>

namespace Grace
{

template <typename SemTy>
Semaphore<SemTy>::~Semaphore()
{
    if (!IsNull())
    {
        vkDestroySemaphore(mDevicePtr->GetVkHandle(), mSemaphore, nullptr);
    }
}

template <typename SemTy>
Semaphore<SemTy>::Semaphore(Device* pDevice, const SemaphoreDesc& desc) : mDevicePtr(pDevice)
{
    VkSemaphoreCreateInfo cinfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    VkSemaphoreTypeCreateInfo tcinfo = {};
    if constexpr (std::is_same_v<SemTy, SemaphoreType::Timeline>)
    {
        tcinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        tcinfo.pNext = nullptr;
        tcinfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        tcinfo.initialValue = desc.initialValue;
        cinfo.pNext = &tcinfo;
    }

    DebugReporter::Check(vkCreateSemaphore(mDevicePtr->GetVkHandle(), &cinfo, nullptr, &mSemaphore));
    AssignDebugName<VkSemaphore>(mDevicePtr->GetVkHandle(), mSemaphore, desc.name);
}

template <typename SemTy>
Semaphore<SemTy>::Semaphore(Semaphore&& other) noexcept : mDevicePtr(other.mDevicePtr), mSemaphore(other.mSemaphore)
{
    other.mSemaphore = VK_NULL_HANDLE;
}

template <typename SemTy>
Semaphore<SemTy>& Semaphore<SemTy>::operator=(Semaphore&& other) noexcept
{
    mDevicePtr = other.mDevicePtr;
    mSemaphore = other.mSemaphore;
    other.mSemaphore = VK_NULL_HANDLE;
    return *this;
}

template <typename SemTy>
bool Semaphore<SemTy>::IsNull() const
{
    return mSemaphore == VK_NULL_HANDLE;
}

template <typename SemTy>
const VkSemaphore& Semaphore<SemTy>::GetVkSemaphore() const
{
    return mSemaphore;
}

template class Semaphore<SemaphoreType::Binary>;
template class Semaphore<SemaphoreType::Timeline>;

} // namespace Grace
