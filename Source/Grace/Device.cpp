#include "Device.hpp"

#include <cassert>
#include <set>

#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif

#include <iostream>

#include <Grace/Context.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/HelperFunctions.hpp>

namespace Grace
{

Device::~Device()
{
    m_Swapchain.reset();
    vkDestroySurfaceKHR(m_ParentInstance, m_SurfaceKHR, nullptr);
    m_ResourceMgr.reset();
    m_ResourceTable.reset();
    m_CmdGroupAllocator.reset();
    m_QueryMgr.reset();
    vmaDestroyAllocator(m_Allocator);
    vkDestroyDevice(m_Device, nullptr);
}

Device::Device(VkInstance instance, const DeviceDesc& desc) : m_ParentInstance(instance)
{
    LogicalDeviceDesc ldd = {};

#ifdef USE_GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    DebugReporter::Check(glfwCreateWindowSurface(instance, desc.pGlfwWindow, nullptr, &m_SurfaceKHR));
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

    ldd.requiredExt = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME };

    // Look for and select a graphics card in the system that supports the features we need
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    assert(deviceCount > 0 && "Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (IsDeviceSuitable(device, ldd.requiredExt))
        {
            m_PhysicalDevice = device;
            break;
        }
    }

    assert(m_PhysicalDevice && "Failed to find a suitable GPU!");

    ConfigureQueues(ldd.queueCreateInfos);

    ConfigureLogicalDevice(ldd);

    vkGetDeviceQueue(m_Device,
                     m_QueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Transfer)].value(),
                     0,
                     &m_Queues[static_cast<uint32_t>(QueueFamily::Transfer)]);
    vkGetDeviceQueue(m_Device,
                     m_QueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Compute)].value(),
                     0,
                     &m_Queues[static_cast<uint32_t>(QueueFamily::Compute)]);
    vkGetDeviceQueue(m_Device,
                     m_QueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Graphics)].value(),
                     0,
                     &m_Queues[static_cast<uint32_t>(QueueFamily::Graphics)]);
    vkGetDeviceQueue(m_Device,
                     m_QueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Present)].value(),
                     0,
                     &m_Queues[static_cast<uint32_t>(QueueFamily::Present)]);

    if (m_Queues[static_cast<uint32_t>(QueueFamily::Transfer)] != nullptr)
    {
        const char* queueDebugName = "Grace::Queue::Transfer";
        AssignDebugName<VkQueue>(m_Device, m_Queues[static_cast<uint32_t>(QueueFamily::Transfer)], queueDebugName);
    }
    if (m_Queues[static_cast<uint32_t>(QueueFamily::Compute)] != nullptr)
    {
        const char* queueDebugName = "Grace::Queue::Compute";
        AssignDebugName<VkQueue>(m_Device, m_Queues[static_cast<uint32_t>(QueueFamily::Compute)], queueDebugName);
    }
    if (m_Queues[static_cast<uint32_t>(QueueFamily::Graphics)] != nullptr)
    {
        const char* queueDebugName = "Grace::Queue::Graphics";
        AssignDebugName<VkQueue>(m_Device, m_Queues[static_cast<uint32_t>(QueueFamily::Graphics)], queueDebugName);
    }
    if (m_Queues[static_cast<uint32_t>(QueueFamily::Present)] != nullptr)
    {
        const char* queueDebugName = "Grace::Queue::Present";
        AssignDebugName<VkQueue>(m_Device, m_Queues[static_cast<uint32_t>(QueueFamily::Present)], queueDebugName);
    }

    // Initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.instance = instance;
    allocatorInfo.physicalDevice = m_PhysicalDevice;
    allocatorInfo.device = m_Device;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &m_Allocator);

    m_QueryMgr = std::make_unique<QueryManager>(this, desc.framesInFlight, desc.queryGroupDesc);
    m_CmdGroupAllocator = std::make_unique<CommandGroupAllocator>(this);
    m_ResourceMgr = std::make_unique<ResourceManager>();
    m_ResourceTable = std::make_unique<GpuResourceTable>(
        this, desc.maxImageDescriptors, desc.maxSamplerDescriptors, desc.maxBufferDescriptors);
}

bool Device::IsNull() const
{
    return m_Device == nullptr || m_Allocator == nullptr;
}

VkDevice Device::GetVkHandle() const
{
    return m_Device;
}

VmaAllocator Device::GetVmaHandle() const
{
    return m_Allocator;
}

void Device::WaitIdle()
{
    vkDeviceWaitIdle(m_Device);
}

void Device::WaitForFences(const std::vector<VkFence>& fences, uint64_t timeout, bool waitAll)
{
    vkWaitForFences(
        m_Device, static_cast<uint32_t>(fences.size()), fences.data(), static_cast<VkBool32>(waitAll), timeout);
}

void Device::ResetFences(const std::vector<VkFence>& fences)
{
    vkResetFences(m_Device, static_cast<uint32_t>(fences.size()), fences.data());
}

void Device::Submit(QueueFamily queue,
                    const std::vector<CommandBuffer>& cmds,
                    const std::vector<VkSemaphore>& waitOn,
                    const std::vector<VkSemaphore>& toSignal,
                    VkFence fence)
{
    uint32_t N = static_cast<uint32_t>(cmds.size());
    std::vector<VkSubmitInfo2> submitInfos(N);

    for (uint32_t i = 0; i < N; ++i)
    {
        VkCommandBufferSubmitInfo cmdInfo = {};
        cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmdInfo.pNext = nullptr;
        cmdInfo.commandBuffer = cmds[i].GetVkCommandBuffer();
        cmdInfo.deviceMask = 0;

        if (!waitOn.empty())
        {
            VkSemaphoreSubmitInfo waitSemaphoreInfo = {};
            waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            waitSemaphoreInfo.pNext = nullptr;
            waitSemaphoreInfo.semaphore = waitOn[i];
            waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            waitSemaphoreInfo.deviceIndex = 0;
            waitSemaphoreInfo.value = 1;

            VkSemaphoreSubmitInfo signalSemaphoreInfo = {};
            signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            signalSemaphoreInfo.pNext = nullptr;
            signalSemaphoreInfo.semaphore = toSignal[i];
            signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            signalSemaphoreInfo.deviceIndex = 0;
            signalSemaphoreInfo.value = 1;

            submitInfos[i] = SubmitInfo(&cmdInfo, &signalSemaphoreInfo, &waitSemaphoreInfo);
        }
        else
        {
            submitInfos[i] = SubmitInfo(&cmdInfo, nullptr, nullptr);
        }
    }

    DebugReporter::Check(vkQueueSubmit2(GetQueue(queue), N, submitInfos.data(), fence));
}

SwapchainStatus Device::Present(VkSemaphore waitSemaphore, uint32_t swapchainImageIndex)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Swapchain->GetVkHandle();
    presentInfo.pImageIndices = &swapchainImageIndex;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(GetQueue(QueueFamily::Present), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        return SwapchainStatus::ShouldResize;
    }
    else if (result != VK_SUCCESS)
    {
        return SwapchainStatus::Failure;
    }

    return SwapchainStatus::Success;
}

VkDescriptorPool& Device::GetSoleDescriptorPool()
{
    return m_ResourceTable->soleDescriptorPool;
}

VkDescriptorSet& Device::GetSoleDescriptorSet()
{
    return m_ResourceTable->soleDescriptorSet;
}

VkDescriptorSetLayout& Device::GetSoleDescriptorSetLayout()
{
    return m_ResourceTable->soleDescriptorSetLayout;
}

VkPipelineLayout& Device::GetSolePipelineLayout()
{
    return m_ResourceTable->solePipelineLayout;
}

void Device::UpdateBindlessDescriptorSet()
{
    m_ResourceTable->UpdateTable();
}

void Device::SubmitImageView(ImageView& view)
{
    m_ResourceTable->SubmitImageView(view);
}

BufferHandle Device::CreateBuffer(const BufferDesc& desc)
{
    BufferHandle newHandle = m_ResourceMgr->Create<Buffer>(this, desc);

    if (desc.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    {
        Buffer& b = m_ResourceMgr->Get<Buffer>(newHandle);
        m_ResourceTable->SubmitBuffer(b);
    }

    return newHandle;
}

Buffer& Device::GetBuffer(const BufferHandle& handle)
{
    return m_ResourceMgr->Get<Buffer>(handle);
}

void Device::FreeBuffer(BufferHandle& handle)
{
    m_ResourceMgr->Free<Buffer>(handle);
}

ImageHandle Device::CreateImage(const ImageDesc& desc)
{
    ImageHandle newHandle = m_ResourceMgr->Create<Image>(this, desc);

    Image& t = m_ResourceMgr->Get<Image>(newHandle);
    m_ResourceTable->SubmitImage(t);

    return newHandle;
}

Image& Device::GetImage(const ImageHandle& handle)
{
    return m_ResourceMgr->Get<Image>(handle);
}

std::vector<RegistryEntry<Image>>& Device::GetAllImages()
{
    return m_ResourceMgr->GetAllImages();
}

void Device::FreeImage(ImageHandle& handle)
{
    m_ResourceTable->FreeImage(m_ResourceMgr->Get<Image>(handle));
    m_ResourceMgr->Free<Image>(handle);
}

SamplerHandle Device::CreateSampler(const SamplerDesc& desc)
{
    SamplerHandle newHandle = m_ResourceMgr->Create<Sampler>(this, desc);

    Sampler& s = m_ResourceMgr->Get<Sampler>(newHandle);
    m_ResourceTable->SubmitSampler(s);

    return newHandle;
}

Sampler& Device::GetSampler(const SamplerHandle& handle)
{
    return m_ResourceMgr->Get<Sampler>(handle);
}

void Device::FreeSampler(SamplerHandle& handle)
{
    m_ResourceTable->FreeSampler(m_ResourceMgr->Get<Sampler>(handle));
    m_ResourceMgr->Free<Sampler>(handle);
}

PipelineHandle Device::CreatePipeline(const PipelineDesc& desc)
{
    return m_ResourceMgr->Create<Pipeline>(this, desc);
}

Pipeline& Device::GetPipeline(const PipelineHandle& handle)
{
    return m_ResourceMgr->Get<Pipeline>(handle);
}

void Device::FreePipeline(PipelineHandle& handle)
{
    m_ResourceMgr->Free<Pipeline>(handle);
}

PipelineLayoutHandle Device::CreatePipelineLayout(const PipelineLayoutDesc& desc)
{
    return m_ResourceMgr->Create<PipelineLayout>(this, desc);
}

PipelineLayout& Device::GetPipelineLayout(const PipelineLayoutHandle& handle)
{
    return m_ResourceMgr->Get<PipelineLayout>(handle);
}

void Device::FreePipelineLayout(PipelineLayoutHandle& handle)
{
    m_ResourceMgr->Free<PipelineLayout>(handle);
}

CommandPool* Device::GetCommandPool(QueueFamily queueFamily, const char* name)
{
    return m_CmdGroupAllocator->GetOrAllocateCommandPool(queueFamily, name);
}

void Device::FreeCommandBuffer(CommandBuffer commandBuffer)
{
    m_CmdGroupAllocator->FreeCommandBuffer();
}

uint32_t Device::GetQueueFamilyIndex(QueueFamily queueFamily)
{
    assert(queueFamily != QueueFamily::Undefined);
    std::optional<uint32_t> queueFamilyIndex = m_QueueFamilyIndices[static_cast<uint32_t>(queueFamily)];
    assert(queueFamilyIndex.has_value());
    return queueFamilyIndex.value();
}

VkQueue Device::GetQueue(QueueFamily queueFamily)
{
    assert(queueFamily != QueueFamily::Undefined);
    std::optional<uint32_t> queueFamilyIndex = m_QueueFamilyIndices[static_cast<uint32_t>(queueFamily)];
    assert(queueFamilyIndex.has_value());
    return m_Queues[static_cast<uint32_t>(queueFamily)];
}

QueryManager* Device::GetQueryManagerPtr()
{
    return m_QueryMgr.get();
}

const QueryGroup&
Device::GetQueryPoolResults(QueryType qt, uint32_t firstQuery, uint32_t queryCount, VkQueryResultFlags flags) const
{
    QueryGroup& qg = m_QueryMgr->GetQueryGroup(qt);

    uint32_t qc = qg.GetQueryCount();
    uint32_t stride = qg.GetValuesPerQuery() * sizeof(uint64_t);

    if ((flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) != 0)
    {
        stride += sizeof(uint64_t);
    }

    uint32_t dataSize = qc * stride;
    DebugReporter::Check(vkGetQueryPoolResults(m_Device,
                                               qg.GetVkQueryPool(),
                                               0,
                                               qc,
                                               dataSize,
                                               qg.GetQueries().data(),
                                               stride,
                                               VK_QUERY_RESULT_64_BIT | flags));

    return qg;
}

FrameSyncGroup& Device::AcquireNextSwapchainImage(VkExtent2D imageExtent)
{
    return m_Swapchain->AcquireNextImage(this, imageExtent);
}

const Image& Device::GetRecentlyAcquiredSwapchainImage() const
{
    return m_Swapchain->GetRecentAcquiredImage();
}

const FrameSyncGroup& Device::GetRecentImageAcquiredDesc()
{
    return m_Swapchain->GetRecentFrameSyncGroup();
}

VkFormat Device::GetSwapchainFormat() const
{
    return m_Swapchain->GetFormat();
}

void Device::CreateSwapchain(VkExtent2D imageExtent, bool vsync)
{
    if (m_Swapchain != nullptr)
    {
        WaitIdle();
        m_Swapchain.reset();
    }

    m_Swapchain = std::make_unique<Swapchain>(this, imageExtent, vsync);
}

SwapchainStatus Device::GetSwapchainStatus() const
{
    return m_Swapchain->GetStatus();
}

VkPhysicalDevice Device::GetPhysicalDevice() const
{
    return m_PhysicalDevice;
}

VkSurfaceKHR Device::GetSurface() const
{
    return m_SurfaceKHR;
}

void Device::ConfigurePhysicalDevice(VkInstance instance, const std::vector<const char*>& requiredExt)
{
    // Look for and select a graphics card in the system that supports the features we need
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    assert(deviceCount > 0 && "Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (IsDeviceSuitable(device, requiredExt))
        {
            m_PhysicalDevice = device;
            break;
        }
    }

    assert(m_PhysicalDevice && "Failed to find a suitable GPU!");
}

void Device::ConfigureLogicalDevice(const LogicalDeviceDesc& desc)
{
    // Specify device features to be used
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = true;

    // Vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features13 = {};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.synchronization2 = true;
    features13.dynamicRendering = true;

    VkPhysicalDeviceVulkan12Features features12 = {};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.pNext = &features13;
    features12.scalarBlockLayout = true;
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;
    features12.descriptorBindingUniformBufferUpdateAfterBind = true;
    features12.descriptorBindingSampledImageUpdateAfterBind = true;
    features12.descriptorBindingStorageImageUpdateAfterBind = true;
    features12.descriptorBindingStorageBufferUpdateAfterBind = true;
    features12.descriptorBindingUniformTexelBufferUpdateAfterBind = true;
    features12.descriptorBindingStorageTexelBufferUpdateAfterBind = true;
    features12.descriptorBindingUpdateUnusedWhilePending = true;
    features12.descriptorBindingPartiallyBound = true;
    features12.descriptorBindingVariableDescriptorCount = true;

    VkPhysicalDeviceVulkan11Features features11 = {};
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features11.pNext = &features12;
    features11.shaderDrawParameters = true;

    VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR compShaderDerivativesFeatures = {};
    compShaderDerivativesFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_KHR;
    compShaderDerivativesFeatures.pNext = &features11;
    compShaderDerivativesFeatures.computeDerivativeGroupQuads = true;
    compShaderDerivativesFeatures.computeDerivativeGroupLinear = true;

    VkPhysicalDeviceFeatures2 features2 = {};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &compShaderDerivativesFeatures;
    features2.features = deviceFeatures;

    vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &features2);

    // Set up a logical device to interface with the physical device
    // Can create multiple logical devices from the same physical device if there are varying requirements
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features2;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(desc.queueCreateInfos.size());
    createInfo.pQueueCreateInfos = desc.queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(desc.requiredExt.size());
    createInfo.ppEnabledExtensionNames = desc.requiredExt.data();
    createInfo.pEnabledFeatures = nullptr;
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;

    DebugReporter::Check(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device));
}

void Device::ConfigureQueues(std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos)
{
    // Fetch physical device's queue family indices

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

    bool minQueueFamilyFound = false;
    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            m_QueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Transfer)] = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            m_QueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Compute)] = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            minQueueFamilyFound = true;
            m_QueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Graphics)] = i;
        }

        if (m_SurfaceKHR != nullptr)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_SurfaceKHR, &presentSupport);

            if (presentSupport)
            {
                m_QueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Present)] = i;
            }
        }

        i++;
    }

    assert(minQueueFamilyFound && "Minimum queue family required (Graphics) not found!");

    // Create a queue for each family
    queueCreateInfos.reserve(static_cast<uint32_t>(QueueFamily::Undefined));
    std::set<uint32_t> uniqueQueueFamilies = {
        m_QueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Transfer)].value(),
        m_QueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Compute)].value(),
        m_QueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Graphics)].value(),
        m_QueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Present)].value()
    };

    // Queue priorities are floats in [0.0, 1.0] - required
    float queuePriority = 1.0F;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
}

bool Device::IsDeviceSuitable(VkPhysicalDevice device, const std::vector<const char*>& requiredExt) const
{
    assert(device != nullptr);

    QueueFamilyIndices indices = FindQueueFamilies(device, m_SurfaceKHR);

    bool bExtensionsSupported = CheckDeviceExtensionSupport(device, requiredExt);

    bool bSwapChainAdequate = false;
    if (bExtensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, m_SurfaceKHR);
        bSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures2 supportedFeatures = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    vkGetPhysicalDeviceFeatures2(device, &supportedFeatures);

    return indices.IsComplete() && bExtensionsSupported && bSwapChainAdequate
        && supportedFeatures.features.samplerAnisotropy;
}

bool Device::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& requiredExt) const
{
    assert(device != nullptr);

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(requiredExt.begin(), requiredExt.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

} // namespace Grace
