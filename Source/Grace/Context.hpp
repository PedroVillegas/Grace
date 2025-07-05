#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <Grace/Device.hpp>
#include <Grace/GraceExport.h>

struct GLFWwindow;

namespace Grace
{

inline PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT_Meta;
inline PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT_Meta;
inline PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT_Meta;
inline PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT_Meta;

#ifdef _DEBUG
#define VVL_ENABLED
#endif

#ifdef VVL_ENABLED
#define VK_SET_DEBUG_NAME vkSetDebugUtilsObjectNameEXT_Meta
#else
#define VK_SET_DEBUG_NAME(...) ((void) 0)
#endif

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

    _NODISCARD Device* GetDevicePtr();

    _NODISCARD VkInstance& GetInstance();

public:
    const std::vector<const char*> REQUIRED_DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                                  VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME };

private:
    void CreateInstance();

    _NODISCARD std::vector<const char*> GetRequiredExtensions() const;

private:
    VkInstance m_Instance = {};
    std::unique_ptr<Device> m_Device = {};
};

} // namespace Grace
