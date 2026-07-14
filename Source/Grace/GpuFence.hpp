#pragma once

#include <vulkan/vulkan.h>
#include <Grace/GraceApi.hpp>
#include <Grace/Detail/Macros.hpp>
#include <Grace/Enums.hpp>
#include <Grace/GpuObjectDesc.hpp>

namespace Grace
{

class Device;

class GRACE_API Fence
{
public:
    ~Fence();
    Fence() = default;
    Fence(Device* pDevice, const GpuFenceDesc& desc);

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkFence handle more than once
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    Fence(Fence&& other) noexcept;
    Fence& operator=(Fence&& other) noexcept;

    GRACE_NODISCARD bool Exists() const;

    GRACE_NODISCARD const VkFence& VkHandle() const;

private:
    Device* mDevicePtr = nullptr;
    VkFence mFence = VK_NULL_HANDLE;
};

} // namespace Grace
