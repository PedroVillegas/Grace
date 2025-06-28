#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <Grace/Device.hpp>
#include <Grace/GraceExport.h>

struct GLFWwindow;

namespace Grace
{

inline PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT_Meta;
inline PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT_Meta;
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

    [[nodiscard]] Device* GetDevicePtr();

    [[nodiscard]] VkInstance& GetInstance();

public:
    const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char*> REQUIRED_DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                                  VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME };

#ifdef VVL_ENABLED
    const bool ENABLE_VALIDATION_LAYERS = true;
#else
    const bool ENABLE_VALIDATION_LAYERS = false;
#endif

    const bool ENABLE_GPU_ASSISTED_VALIDATION = false;

private:
    void CreateInstance();
    void SetupDebugMessenger();

    [[nodiscard]] bool CheckValidationLayerSupport() const;
    [[nodiscard]] std::vector<const char*> GetRequiredExtensions() const;

private:
    VkInstance m_Instance = {};
    VkDebugUtilsMessengerEXT m_DebugMessenger = {};
    std::unique_ptr<Device> m_Device = {};
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData);

} // namespace Grace
