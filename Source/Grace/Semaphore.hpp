#pragma once

#include <concepts>

#include <vulkan/vulkan.h>
#include <Grace/GraceExport.h>
#include <Grace/Macros.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/HelperFunctions.hpp>

namespace Grace
{

class Device;

namespace SemaphoreType
{

struct SemaphoreTypeBase
{
};

struct GRACE_EXPORT Binary : SemaphoreTypeBase
{
};

struct GRACE_EXPORT Timeline : SemaphoreTypeBase
{
};

} // namespace SemaphoreType

struct GRACE_EXPORT SemaphoreDesc
{
    const char* name = "";
    uint64_t initialValue = 0;
};

template <typename SemTy>
class GRACE_EXPORT Semaphore
{
    static_assert(std::derived_from<SemTy, SemaphoreType::SemaphoreTypeBase>,
                  "Semaphore type must be derived from SemaphoreType::SemaphoreTypeBase!");

public:
    ~Semaphore()
    {
        if (!IsNull())
        {
            vkDestroySemaphore(m_pDevice->GetVkHandle(), m_Semaphore, nullptr);
        }
    }

    Semaphore() = default;

    Semaphore(Device* pDevice, const SemaphoreDesc& desc) : m_pDevice(pDevice)
    {
        VkSemaphoreCreateInfo cinfo = {};
        cinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        cinfo.pNext = nullptr;
        cinfo.flags = 0;

        VkSemaphoreTypeCreateInfo tcinfo = {};
        if constexpr (std::is_same_v<SemTy, SemaphoreType::Timeline>)
        {
            tcinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            tcinfo.pNext = nullptr;
            tcinfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            tcinfo.initialValue = desc.initialValue;
            cinfo.pNext = &tcinfo;
        }

        DebugReporter::Check(vkCreateSemaphore(m_pDevice->GetVkHandle(), &cinfo, nullptr, &m_Semaphore));
        AssignDebugName<VkSemaphore>(m_pDevice->GetVkHandle(), m_Semaphore, desc.name);
    }

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkSemaphore handle more than once
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    Semaphore(Semaphore&& other) noexcept : m_pDevice(other.m_pDevice), m_Semaphore(other.m_Semaphore)
    {
        other.m_Semaphore = VK_NULL_HANDLE;
    }

    Semaphore& operator=(Semaphore&& other) noexcept
    {
        m_pDevice = other.m_pDevice;
        m_Semaphore = other.m_Semaphore;
        other.m_Semaphore = VK_NULL_HANDLE;
        return *this;
    }

    GRACE_NODISCARD bool IsNull() const
    {
        return m_Semaphore == VK_NULL_HANDLE;
    }

    GRACE_NODISCARD VkSemaphore GetVkSemaphore() const
    {
        return m_Semaphore;
    }

private:
    Device* m_pDevice = nullptr;
    VkSemaphore m_Semaphore = VK_NULL_HANDLE;
};

using BinarySemaphore = Semaphore<SemaphoreType::Binary>;
using TimelineSemaphore = Semaphore<SemaphoreType::Timeline>;

} // namespace Grace
