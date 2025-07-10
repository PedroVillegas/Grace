#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <Grace/Device.hpp>
#include <Grace/GraceExport.h>

struct GLFWwindow;

namespace Grace
{

struct ContextDesc
{
    DeviceDesc deviceConfig;
};

class GRACE_EXPORT Context
{
public:
    ~Context();
    Context() = default;
    explicit Context(const ContextDesc& desc);

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    Context(Context&&) noexcept = delete;
    Context& operator=(Context&&) noexcept = delete;

    GRACE_NODISCARD Device* GetDevicePtr();

    GRACE_NODISCARD VkInstance& GetInstance();

public:
    const std::vector<const char*> REQUIRED_DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                                  VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME };

private:
    void CreateInstance();

    GRACE_NODISCARD std::vector<const char*> GetRequiredExtensions() const;

private:
    VkInstance m_Instance = {};
    std::unique_ptr<Device> m_Device = {};
};

} // namespace Grace
