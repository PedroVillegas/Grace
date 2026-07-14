#pragma once

#include <Grace/GraceApi.hpp>
#include <Grace/Detail/Macros.hpp>
#include <Grace/HelperFunctions.hpp>
#include <Grace/GpuObjectDesc.hpp>
#include <Grace/GpuObjectTraits.hpp>

namespace Grace
{

class Device;

template <SemaphoreVariant T>
class GRACE_API Semaphore
{
public:
    ~Semaphore();
    Semaphore() = default;
    Semaphore(Device* pDevice, const GpuObjectDesc<Semaphore<T>>& desc);

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkSemaphore handle more than once
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    Semaphore(Semaphore&& other) noexcept;
    Semaphore& operator=(Semaphore&& other) noexcept;

    GRACE_NODISCARD bool Exists() const;

    GRACE_NODISCARD const VkSemaphore& VkHandle() const;

private:
    Device* mDevicePtr = nullptr;
    VkSemaphore mSemaphore = VK_NULL_HANDLE;
};

using BinarySemaphore = Semaphore<SemaphoreType::Binary>;
using TimelineSemaphore = Semaphore<SemaphoreType::Timeline>;

} // namespace Grace
