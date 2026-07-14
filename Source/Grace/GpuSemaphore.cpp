#include <Grace/GpuSemaphore.hpp>

#include <Grace/Device.hpp>

namespace Grace
{

template <SemaphoreVariant T>
Semaphore<T>::~Semaphore()
{
    if (!Exists())
    {
        vkDestroySemaphore(mDevicePtr->VkHandle(), mSemaphore, nullptr);
    }
}

template <SemaphoreVariant T>
Semaphore<T>::Semaphore(Device* pDevice, const GpuObjectDesc<Semaphore<T>>& desc) : mDevicePtr(pDevice)
{
    VkSemaphoreCreateInfo cinfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    VkSemaphoreTypeCreateInfo tcinfo = {};
    if constexpr (std::is_same_v<T, SemaphoreType::Timeline>)
    {
        tcinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        tcinfo.pNext = nullptr;
        tcinfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        tcinfo.initialValue = desc.initialValue;
        cinfo.pNext = &tcinfo;
    }

    DebugReporter::Check(vkCreateSemaphore(mDevicePtr->VkHandle(), &cinfo, nullptr, &mSemaphore));
    AssignDebugName<VkSemaphore>(mDevicePtr->VkHandle(), mSemaphore, desc.name);
}

template <SemaphoreVariant T>
Semaphore<T>::Semaphore(Semaphore&& other) noexcept : mDevicePtr(other.mDevicePtr), mSemaphore(other.mSemaphore)
{
    other.mSemaphore = VK_NULL_HANDLE;
}

template <SemaphoreVariant T>
Semaphore<T>& Semaphore<T>::operator=(Semaphore&& other) noexcept
{
    mDevicePtr = other.mDevicePtr;
    mSemaphore = other.mSemaphore;
    other.mSemaphore = VK_NULL_HANDLE;
    return *this;
}

template <SemaphoreVariant T>
bool Semaphore<T>::Exists() const
{
    return mSemaphore == VK_NULL_HANDLE;
}

template <SemaphoreVariant T>
const VkSemaphore& Semaphore<T>::VkHandle() const
{
    return mSemaphore;
}

template class Semaphore<SemaphoreType::Binary>;
template class Semaphore<SemaphoreType::Timeline>;

} // namespace Grace
