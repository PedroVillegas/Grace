#pragma once

#include <concepts>

#include <Grace/GraceApi.hpp>
#include <Grace/Macros.hpp>
#include <Grace/HelperFunctions.hpp>
#include <Grace/GpuResourceDescriptions.hpp>

namespace Grace
{

class Device;

namespace SemaphoreType
{

struct SemaphoreTypeBase
{};

struct GRACE_API Binary : SemaphoreTypeBase
{};

struct GRACE_API Timeline : SemaphoreTypeBase
{};

} // namespace SemaphoreType

struct GRACE_API SemaphoreDesc
{
    const char* name = "";
    uint64_t initialValue = 0;
};

template <typename SemTy>
class GRACE_API Semaphore
{
    static_assert(std::derived_from<SemTy, SemaphoreType::SemaphoreTypeBase>,
                  "Semaphore type must be one of SemaphoreType::Binary or SemaphoreType::Timeline!");

public:
    ~Semaphore();
    Semaphore() = default;
    Semaphore(Device* pDevice, const GpuResourceDesc<Semaphore<SemTy>>& desc);

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
