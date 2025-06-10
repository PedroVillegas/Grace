#include "Context.hpp"

#include <Grace/DebugReporter.hpp>
#include <Grace/HelperFunctions.hpp>

#include <iostream>
#include <cassert>
#include <cstring>

#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif

namespace Grace
{

VkInstance& Context::GetInstance()
{
    return m_Instance;
}

Context::~Context()
{
    m_Device.WaitIdle();

    if (ENABLE_VALIDATION_LAYERS)
    {
        vkDestroyDebugUtilsMessengerEXT_Meta(m_Instance, m_DebugMessenger, nullptr);
    }

    m_Device.Cleanup(m_Instance);
    vkDestroyInstance(m_Instance, nullptr);
}

void Context::Initialise(const ContextDesc& desc)
{
    vkCreateDebugUtilsMessengerEXT_Meta = nullptr;
    vkDestroyDebugUtilsMessengerEXT_Meta = nullptr;
    vkSetDebugUtilsObjectNameEXT_Meta = nullptr;

    CreateInstance();
    SetupDebugMessenger();

    m_Device.Initialise(m_Instance, desc.deviceConfig);
}

Device* Context::GetDevicePtr()
{
    return &m_Device;
}

void Context::CreateInstance()
{
    // Initialize the Vulkan library by creating a Vulkan Instance

    if (ENABLE_VALIDATION_LAYERS && !CheckValidationLayerSupport())
    {
        std::cout << "Validation layers requested, but not available!\n";
    }

    VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
                                               VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT };
    VkValidationFeaturesEXT features = {};
    features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    features.enabledValidationFeatureCount = 2;
    features.pEnabledValidationFeatures = enables;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Tensa";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = ENABLE_GPU_ASSISTED_VALIDATION ? &features : nullptr;

    // Define the global extensions and validation layers we want to use
    std::vector<const char*> extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    if (ENABLE_VALIDATION_LAYERS)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        debugCreateInfo.pNext = ENABLE_GPU_ASSISTED_VALIDATION ? &features : nullptr;
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }

    DebugReporter::Check(vkCreateInstance(&createInfo, nullptr, &m_Instance));

    if (ENABLE_VALIDATION_LAYERS)
    {
        vkCreateDebugUtilsMessengerEXT_Meta =
            (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
        vkDestroyDebugUtilsMessengerEXT_Meta =
            (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
        vkSetDebugUtilsObjectNameEXT_Meta =
            (PFN_vkSetDebugUtilsObjectNameEXT) vkGetInstanceProcAddr(m_Instance, "vkSetDebugUtilsObjectNameEXT");
        assert(vkCreateDebugUtilsMessengerEXT_Meta && vkDestroyDebugUtilsMessengerEXT_Meta
               && vkSetDebugUtilsObjectNameEXT_Meta);
    }
}

void Context::SetupDebugMessenger()
{
    if (!ENABLE_VALIDATION_LAYERS)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    PopulateDebugMessengerCreateInfo(createInfo);

    DebugReporter::Check(vkCreateDebugUtilsMessengerEXT_Meta(m_Instance, &createInfo, nullptr, &m_DebugMessenger));
}

bool Context::CheckValidationLayerSupport() const
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : VALIDATION_LAYERS)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

std::vector<const char*> Context::GetRequiredExtensions() const
{
#ifdef USE_GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
#endif

    // Checking for supported extensions
    uint32_t extensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);

    std::vector<VkExtensionProperties> availableInstanceExtensions(extensionsCount);

    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableInstanceExtensions.data());

    std::vector<const char*> extensions
#ifdef USE_GLFW
        (glfwExtensions, glfwExtensions + glfwExtensionCount)
#endif
            ;

    for (auto& availableExt : availableInstanceExtensions)
    {
        if (strcmp(availableExt.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void Context::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const
{
    createInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                   .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                   .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                   .pfnUserCallback = DebugCallback };
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        // Message is important enough to show
        std::cout << "\033[1;31m[ERROR]\033[0m " << pCallbackData->pMessage << "\n";
    }

    return VK_FALSE;
}

} // namespace Grace
