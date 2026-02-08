#pragma once

#include <vulkan/vulkan.h>
#include <Grace/GraceApi.hpp>
#include <Grace/Macros.hpp>
#include <Grace/Enums.hpp>

namespace Grace
{

class Device;

struct GRACE_API FenceDesc
{
    const char* name = "";
    FenceFlags flags = {};
};

class GRACE_API Fence
{
public:
    ~Fence();
    Fence() = default;
    Fence(Device* pDevice, const FenceDesc& desc);

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkFence handle more than once
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    Fence(Fence&& other) noexcept;
    Fence& operator=(Fence&& other) noexcept;

    GRACE_NODISCARD bool IsNull() const;

    GRACE_NODISCARD const VkFence& GetVkFence() const;

private:
    Device* mDevicePtr = nullptr;
    VkFence mFence = VK_NULL_HANDLE;
};

} // namespace Grace
