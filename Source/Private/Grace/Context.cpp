#include <Grace/Context.hpp>

#include <Grace/DebugReporter.hpp>
#include <Grace/HelperFunctions.hpp>
#include <Private/Grace/ScratchVector.hpp>

#include <cstring>

namespace Grace
{

GRACE_NODISCARD static ScratchVector<const char*> GetRequiredExtensions()
{
    // Checking for supported extensions
    uint32_t extensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);

    ScratchVector<VkExtensionProperties> availableInstanceExtensions(extensionsCount);

    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableInstanceExtensions.data());

    ScratchVector<const char*> extensions;

    for (auto& availableExt : availableInstanceExtensions)
    {
        if (strcmp(availableExt.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

VkInstance Context::GetInstance()
{
    return mInstance;
}

Context::~Context()
{
    mDevice->WaitIdle();
    mDevice.reset();
    vkDestroyInstance(mInstance, nullptr);
}

Context::Context(const ContextDesc& desc)
{
    vkSetDebugUtilsObjectNameEXT_Meta = nullptr;
    vkCmdBeginDebugUtilsLabelEXT_Meta = nullptr;
    vkCmdEndDebugUtilsLabelEXT_Meta = nullptr;
    vkCmdInsertDebugUtilsLabelEXT_Meta = nullptr;

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

#if (GRACE_TARGET_VULKAN_API_VERSION == 13)
    appInfo.apiVersion = VK_API_VERSION_1_3;
#elif (GRACE_TARGET_VULKAN_API_VERSION == 14)
    appInfo.apiVersion = VK_API_VERSION_1_4;
#else
    vkEnumerateInstanceVersion(&apiVersion);
    assert(apiVersion >= VK_API_VERSION_1_3);
#endif

    VkInstanceCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
    };

    ScratchVector<const char*> extensions = GetRequiredExtensions();
    extensions.insert(extensions.end(), desc.extensions.begin(), desc.extensions.end());
    ici.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    ici.ppEnabledExtensionNames = extensions.data();

    DebugReporter::Check(vkCreateInstance(&ici, nullptr, &mInstance));

    vkSetDebugUtilsObjectNameEXT_Meta = GRACE_LOAD_INSTANCE_PFN(mInstance, vkSetDebugUtilsObjectNameEXT);
    vkCmdBeginDebugUtilsLabelEXT_Meta = GRACE_LOAD_INSTANCE_PFN(mInstance, vkCmdBeginDebugUtilsLabelEXT);
    vkCmdEndDebugUtilsLabelEXT_Meta = GRACE_LOAD_INSTANCE_PFN(mInstance, vkCmdEndDebugUtilsLabelEXT);
    vkCmdInsertDebugUtilsLabelEXT_Meta = GRACE_LOAD_INSTANCE_PFN(mInstance, vkCmdInsertDebugUtilsLabelEXT);
}

Device* Context::DevicePtr(const DeviceDesc& deviceConfig)
{
    mDevice = std::make_unique<Device>(mInstance, deviceConfig);

    AssignDebugName<VkInstance>(mDevice->GetVkHandle(), mInstance, "Grace::Instance");
    AssignDebugName<VkDevice>(mDevice->GetVkHandle(), mDevice->GetVkHandle(), "Grace::Device");

    return mDevice.get();
}

} // namespace Grace
