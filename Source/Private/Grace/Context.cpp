#include <Grace/Context.hpp>

#include <Grace/DebugReporter.hpp>
#include <Grace/HelperFunctions.hpp>
#include <Private/Grace/ScratchVector.hpp>
#include <Private/Grace/Config.hpp>
#include <Private/Grace/InternalContainers.hpp>

#include <cstring>

namespace Grace
{

VkInstance Context::GetInstance()
{
    return mInstance;
}

void Context::Startup()
{
    gAbandonedResources = std::make_unique<AbandonedResources>();
    gResHandleRefCounters = std::make_unique<ResourceHandleRefCounters>();

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

    ScratchVector<const char*> instanceExtensions = {};
    instanceExtensions.reserve(gConfig.InstanceExtensions.size());
    for (const std::string& ext : gConfig.InstanceExtensions)
    {
        instanceExtensions.push_back(ext.c_str());
    }

    ScratchVector<const char*> instanceLayers = {};
    instanceLayers.reserve(gConfig.InstanceLayers.size());
    for (const std::string& layer : gConfig.InstanceLayers)
    {
        instanceLayers.push_back(layer.c_str());
    }

    const VkInstanceCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(instanceLayers.size()),
        .ppEnabledLayerNames = instanceLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data(),
    };

    DebugReporter::Check(vkCreateInstance(&ici, nullptr, &mInstance));

    vkSetDebugUtilsObjectNameEXT_Meta = GRACE_LOAD_INSTANCE_PFN(mInstance, vkSetDebugUtilsObjectNameEXT);
    vkCmdBeginDebugUtilsLabelEXT_Meta = GRACE_LOAD_INSTANCE_PFN(mInstance, vkCmdBeginDebugUtilsLabelEXT);
    vkCmdEndDebugUtilsLabelEXT_Meta = GRACE_LOAD_INSTANCE_PFN(mInstance, vkCmdEndDebugUtilsLabelEXT);
    vkCmdInsertDebugUtilsLabelEXT_Meta = GRACE_LOAD_INSTANCE_PFN(mInstance, vkCmdInsertDebugUtilsLabelEXT);
}

Context::~Context()
{
    mDevice->WaitIdle();
    mDevice.reset();
    vkDestroyInstance(mInstance, nullptr);
    gAbandonedResources.reset();
    gResHandleRefCounters.reset();
}

Context::Context()
{
    Startup();
}

Context::Context(const std::string& yaml)
{
    ProcessYamlConfig(yaml);
    Startup();
}

Device* Context::DevicePtr(VkSurfaceKHR surface)
{
    mDevice = std::make_unique<Device>(mInstance, surface);

    AssignDebugName<VkInstance>(mDevice->GetVkHandle(), mInstance, "Grace::Instance");
    AssignDebugName<VkDevice>(mDevice->GetVkHandle(), mDevice->GetVkHandle(), "Grace::Device");

    return mDevice.get();
}

} // namespace Grace
