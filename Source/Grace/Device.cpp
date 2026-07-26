#include <Grace/Device.hpp>

#include <Grace/DebugReporter.hpp>
#include <Grace/HelperFunctions.hpp>
#include <Grace/CommandGroup.hpp>
#include <Grace/Detail/ScratchVector.hpp>
#include <Grace/Detail/Config.hpp>
#include <Grace/Detail/InternalContainers.hpp>
#include <Grace/Detail/Assert.hpp>
#include <Grace/GpuBindlessTable.hpp>
#include <Grace/GpuObjectManager.hpp>

#include <set>

namespace Grace
{

Device::~Device()
{
    if (mSurfaceKHR != VK_NULL_HANDLE)
    {
        mSwapchain.reset();
        vkDestroySurfaceKHR(mParentInstance, mSurfaceKHR, nullptr);
    }

    mGpuObjManager.reset();
    mResourceTable.reset();
    mCmdGroupAllocator.reset();
    mQueryMgr.reset();
    vmaDestroyAllocator(mAllocator);
    vkDestroyDevice(mDevice, nullptr);
}

Device::Device() = default;

Device::Device(VkInstance instance, VkSurfaceKHR surface)
    : mParentInstance(instance), mFramesInFlight(gConfig.FramesInFlight), mSurfaceKHR(surface)
{
    GRACE_ASSERT(mFramesInFlight > 0);

    LogicalDeviceDesc ldd = {};

    // Look for and select a graphics card in the system that supports the features we need
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    GRACE_ASSERT_MSG(deviceCount > 0, "Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (IsDeviceSuitable(device, ldd.requiredExt))
        {
            mPhysicalDevice = device;
            break;
        }
    }

    GRACE_ASSERT_MSG(mPhysicalDevice, "Failed to find a suitable GPU!");

    ConfigureQueues(ldd.queueCreateInfos);

    ConfigureLogicalDevice(ldd);

    vkGetDeviceQueue(mDevice,
                     mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Transfer)].value(),
                     0,
                     &mQueues[static_cast<uint32_t>(QueueFamily::Transfer)]);
    vkGetDeviceQueue(mDevice,
                     mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Compute)].value(),
                     0,
                     &mQueues[static_cast<uint32_t>(QueueFamily::Compute)]);
    vkGetDeviceQueue(mDevice,
                     mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Graphics)].value(),
                     0,
                     &mQueues[static_cast<uint32_t>(QueueFamily::Graphics)]);

    if (mSurfaceKHR != nullptr)
    {
        vkGetDeviceQueue(mDevice,
                         mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Present)].value(),
                         0,
                         &mQueues[static_cast<uint32_t>(QueueFamily::Present)]);
    }

    if (mQueues[static_cast<uint32_t>(QueueFamily::Transfer)] != nullptr)
    {
        const char* queueDebugName = "Grace::Queue::Transfer";
        AssignDebugName<VkQueue>(mDevice, mQueues[static_cast<uint32_t>(QueueFamily::Transfer)], queueDebugName);
    }
    if (mQueues[static_cast<uint32_t>(QueueFamily::Compute)] != nullptr)
    {
        const char* queueDebugName = "Grace::Queue::Compute";
        AssignDebugName<VkQueue>(mDevice, mQueues[static_cast<uint32_t>(QueueFamily::Compute)], queueDebugName);
    }
    if (mQueues[static_cast<uint32_t>(QueueFamily::Graphics)] != nullptr)
    {
        const char* queueDebugName = "Grace::Queue::Graphics";
        AssignDebugName<VkQueue>(mDevice, mQueues[static_cast<uint32_t>(QueueFamily::Graphics)], queueDebugName);
    }
    if (mQueues[static_cast<uint32_t>(QueueFamily::Present)] != nullptr)
    {
        const char* queueDebugName = "Grace::Queue::Present";
        AssignDebugName<VkQueue>(mDevice, mQueues[static_cast<uint32_t>(QueueFamily::Present)], queueDebugName);
    }

    // Initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.instance = instance;
    allocatorInfo.physicalDevice = mPhysicalDevice;
    allocatorInfo.device = mDevice;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &mAllocator);

    mQueryMgr = std::make_unique<QueryManager>(this);
    mCmdGroupAllocator = std::make_unique<CommandGroupAllocator>(this);
    mGpuObjManager = std::make_unique<GpuObjectManager>();
    mResourceTable = std::make_unique<GpuResourceTable>(this);

    mSingleTimeCmdsPool = GetCommandPool(QueueFamily::Graphics, "Grace::CommandPool::SingleTimeCommands");
    mSingleTimeCmdsBuffer = std::make_unique<CommandBuffer>(mSingleTimeCmdsPool->GetOrAllocateCommandBuffer());
}

bool Device::IsNull() const
{
    return mDevice == nullptr || mAllocator == nullptr;
}

VkDevice Device::VkHandle() const
{
    return mDevice;
}

VmaAllocator Device::GetVmaHandle() const
{
    return mAllocator;
}

void Device::WaitIdle()
{
    vkDeviceWaitIdle(mDevice);
}

uint32_t Device::GetCurrentFrameInFlightIndex() const
{
    return mFrameInFlightIndex;
}

void Device::AdvanceToNextFrame()
{
    if (gAbandonedResources->any)
    {
        for (auto& image : gAbandonedResources->images)
        {
            FreeDeferred<Image>(image);
        }
        for (auto& sampler : gAbandonedResources->samplers)
        {
            FreeDeferred<Sampler>(sampler);
        }
        gAbandonedResources->images.clear();
        gAbandonedResources->samplers.clear();
        gAbandonedResources->any = false;
    }

    mFrameInFlightIndex = (mFrameInFlightIndex + 1) % mFramesInFlight;
}

void Device::WaitForFence(FenceHandle fence, uint64_t timeout)
{
    const Fence& waitFor = Get<Fence>(fence);
    vkWaitForFences(mDevice, 1, &waitFor.VkHandle(), VK_TRUE, timeout);
    mGpuObjManager->FlushDeletionQueue(mFrameInFlightIndex);
}

void Device::WaitForFences(const std::initializer_list<VkFence>&& fences, uint64_t timeout, bool waitAll)
{
    vkWaitForFences(mDevice, static_cast<uint32_t>(fences.size()), fences.begin(), waitAll, timeout);
    mGpuObjManager->FlushDeletionQueue(mFrameInFlightIndex);
}

void Device::ResetFence(FenceHandle fence)
{
    const Fence& toReset = Get<Fence>(fence);
    vkResetFences(mDevice, 1, &toReset.VkHandle());
}

void Device::ResetFences(const std::initializer_list<VkFence>&& fences)
{
    vkResetFences(mDevice, static_cast<uint32_t>(fences.size()), fences.begin());
}

void Device::SubmitAndWait(QueueFamily queueFamily, const CommandBuffer& cmd)
{
    VkCommandBufferSubmitInfo cmdInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = nullptr,
        .commandBuffer = cmd.GetVkCommandBuffer(),
        .deviceMask = 0,
    };

    const VkSubmitInfo2 submitInfo = SubmitInfo(&cmdInfo, nullptr, nullptr);

    DebugReporter::Check(vkQueueSubmit2(GetQueue(queueFamily), 1, &submitInfo, nullptr));
    WaitIdle();
}

void Device::Submit(QueueFamily queue, const CommandBuffer& cmd, const FrameSyncGroup& fsg, FenceHandle fence)
{
    VkCommandBufferSubmitInfo cmdInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                          .pNext = nullptr,
                                          .commandBuffer = cmd.GetVkCommandBuffer(),
                                          .deviceMask = 0 };

    VkSemaphoreSubmitInfo waitSemaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = Get<BinarySemaphore>(fsg.acquireSemaphore).VkHandle(),
        .value = 1,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .deviceIndex = 0,
    };

    VkSemaphoreSubmitInfo signalSemaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = Get<BinarySemaphore>(fsg.presentSemaphore).VkHandle(),
        .value = 1,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .deviceIndex = 0,
    };

    const VkSubmitInfo2 submitInfo = SubmitInfo(&cmdInfo, &signalSemaphoreInfo, &waitSemaphoreInfo);

    const Fence& fenceToSignal = Get<Fence>(fence);
    DebugReporter::Check(vkQueueSubmit2(GetQueue(queue), 1, &submitInfo, fenceToSignal.VkHandle()));
}

void Device::BatchSubmit(QueueFamily queue,
                         std::initializer_list<CommandBuffer> cmds,
                         std::initializer_list<FrameSyncGroup> fsgs,
                         FenceHandle fence)
{
    GRACE_ASSERT(cmds.size() == fsgs.size());

    const uint32_t N = cmds.size();
    ScratchVector<VkSubmitInfo2> submitInfos(N);

    for (uint32_t i = 0; i < N; ++i)
    {
        VkCommandBufferSubmitInfo cmdInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = (cmds.begin() + i)->GetVkCommandBuffer(),
            .deviceMask = 0,
        };

        VkSemaphoreSubmitInfo waitSemaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = Get<BinarySemaphore>((fsgs.begin() + i)->acquireSemaphore).VkHandle(),
            .value = 1,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .deviceIndex = 0,
        };

        VkSemaphoreSubmitInfo signalSemaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = Get<BinarySemaphore>((fsgs.begin() + i)->presentSemaphore).VkHandle(),
            .value = 1,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .deviceIndex = 0,
        };

        submitInfos.push_back(SubmitInfo(&cmdInfo, &signalSemaphoreInfo, &waitSemaphoreInfo));
    }

    const Fence& fenceToSignal = Get<Fence>(fence);
    DebugReporter::Check(vkQueueSubmit2(GetQueue(queue), N, submitInfos.data(), fenceToSignal.VkHandle()));
}

SwapchainStatus Device::Present(const FrameSyncGroup& fsg)
{
    const VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &Get<BinarySemaphore>(fsg.presentSemaphore).VkHandle(),
        .swapchainCount = 1,
        .pSwapchains = &mSwapchain->VkHandle(),
        .pImageIndices = &fsg.imageIndex,
        .pResults = nullptr,
    };

    const VkResult result = vkQueuePresentKHR(GetQueue(QueueFamily::Present), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        return SwapchainStatus::ShouldResize;
    }

    if (result != VK_SUCCESS)
    {
        return SwapchainStatus::Failure;
    }

    return SwapchainStatus::Success;
}

VkDescriptorPool& Device::GetSoleDescriptorPool()
{
    return mResourceTable->bindlessDescriptorPool;
}

VkDescriptorSet& Device::GetSoleDescriptorSet()
{
    return mResourceTable->bindlessDescriptorSet;
}

VkDescriptorSetLayout& Device::GetSoleDescriptorSetLayout()
{
    return mResourceTable->bindlessDescriptorSetLayout;
}

PipelineLayoutHandle Device::GetSolePipelineLayout()
{
    return mResourceTable->bindlessPipelineLayout;
}

void Device::UpdateBindlessDescriptorSet()
{
    mResourceTable->UpdateTable();
}

void Device::CopyMemoryToHostVisibleBuffer(BufferHandle dst,
                                           VkDeviceSize offsetIntoDst,
                                           const void* pHostMem,
                                           VkDeviceSize hostMemBytes)
{
    vmaCopyMemoryToAllocation(mAllocator, pHostMem, Get<Buffer>(dst).GetAllocation(), offsetIntoDst, hostMemBytes);
}

void Device::CopyMemoryToHostVisibleImage(ImageHandle dst,
                                          VkDeviceSize offsetIntoDst,
                                          const void* pHostMem,
                                          VkDeviceSize hostMemBytes)
{
    vmaCopyMemoryToAllocation(mAllocator, pHostMem, Get<Image>(dst).GetAllocation(), offsetIntoDst, hostMemBytes);
}

void Device::SubmitImageView(ImageView& view)
{
    mResourceTable->SubmitImageView(view);
}

ImageHandle Device::CreateSwapchainImage(VkImage image, const GpuImageDesc& desc)
{
    return mGpuObjManager->CreateSwapchainImage(this, image, desc);
}

CommandPool* Device::GetCommandPool(QueueFamily queueFamily, const char* name)
{
    return mCmdGroupAllocator->GetOrAllocateCommandPool(queueFamily, name);
}

void Device::FreeCommandBuffer(CommandBuffer commandBuffer)
{
    mCmdGroupAllocator->FreeCommandBuffer();
}

CommandBuffer& Device::BeginSingleTimeCommands()
{
    mSingleTimeCmdsBuffer->BeginRecording();
    return *mSingleTimeCmdsBuffer;
}

void Device::EndAndSubmitSingleTimeCommands()
{
    mSingleTimeCmdsBuffer->EndRecording();
    SubmitAndWait(QueueFamily::Graphics, *mSingleTimeCmdsBuffer);
    mSingleTimeCmdsPool->Reset();
}

uint32_t Device::GetQueueFamilyIndex(QueueFamily queueFamily)
{
    GRACE_ASSERT(queueFamily != QueueFamily::Undefined);
    std::optional<uint32_t> queueFamilyIndex = mQueueFamilyIndices[static_cast<uint32_t>(queueFamily)];
    GRACE_ASSERT(queueFamilyIndex.has_value());
    return queueFamilyIndex.value();
}

VkQueue Device::GetQueue(QueueFamily queueFamily)
{
    GRACE_ASSERT(queueFamily != QueueFamily::Undefined);
    std::optional<uint32_t> queueFamilyIndex = mQueueFamilyIndices[static_cast<uint32_t>(queueFamily)];
    GRACE_ASSERT(queueFamilyIndex.has_value());
    return mQueues[static_cast<uint32_t>(queueFamily)];
}

QueryManager* Device::GetQueryManagerPtr()
{
    return mQueryMgr.get();
}

FrameSyncGroup& Device::AcquireNextSwapchainImage(VkExtent2D imageExtent)
{
    return mSwapchain->AcquireNextImage(imageExtent);
}

ImageHandle Device::GetRecentlyAcquiredSwapchainImage() const
{
    return mSwapchain->GetRecentAcquiredImage();
}

const FrameSyncGroup& Device::GetRecentImageAcquiredDesc()
{
    return mSwapchain->GetRecentFrameSyncGroup();
}

const Format& Device::GetSwapchainFormat() const
{
    return mSwapchain->GetFormat();
}

void Device::CreateSwapchain(VkExtent2D imageExtent, bool vsync)
{
    if (mSwapchain != nullptr)
    {
        WaitIdle();
        mSwapchain.reset();
    }

    mSwapchain = std::make_unique<Swapchain>(this, imageExtent, vsync);
}

SwapchainStatus Device::GetSwapchainStatus() const
{
    return mSwapchain->GetStatus();
}

VkPhysicalDevice Device::GetPhysicalDevice() const
{
    return mPhysicalDevice;
}

VkSurfaceKHR Device::GetSurface() const
{
    return mSurfaceKHR;
}

void Device::ConfigurePhysicalDevice(VkInstance instance, const std::vector<const char*>& requiredExt)
{
    // Look for and select a graphics card in the system that supports the features we need
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    GRACE_ASSERT_MSG(deviceCount > 0, "Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (IsDeviceSuitable(device, requiredExt))
        {
            mPhysicalDevice = device;
            break;
        }
    }

    GRACE_ASSERT_MSG(mPhysicalDevice, "Failed to find a suitable GPU!");
}

void Device::ConfigureLogicalDevice(const LogicalDeviceDesc& desc)
{
    // Vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features13 = {};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.pNext = nullptr;
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

    vkGetPhysicalDeviceFeatures2(mPhysicalDevice, &features2);

    ScratchVector<const char*> deviceExtensions = {};
    deviceExtensions.reserve(gConfig.DeviceExtensions.size());
    for (const std::string& ext : gConfig.DeviceExtensions)
    {
        deviceExtensions.push_back(ext.c_str());
    }

    ScratchVector<const char*> deviceLayers = {};
    deviceLayers.reserve(gConfig.DeviceLayers.size());
    for (const std::string& layer : gConfig.DeviceLayers)
    {
        deviceLayers.push_back(layer.c_str());
    }

    // Set up a logical device to interface with the physical device
    // Can create multiple logical devices from the same physical device if there are varying requirements
    const VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features2,
        .flags = 0,
        .queueCreateInfoCount = static_cast<uint32_t>(desc.queueCreateInfos.size()),
        .pQueueCreateInfos = desc.queueCreateInfos.data(),
        .enabledLayerCount = static_cast<uint32_t>(deviceLayers.size()),
        .ppEnabledLayerNames = deviceLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = nullptr,
    };

    DebugReporter::Check(vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice));
}

void Device::ConfigureQueues(std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos)
{
    // Fetch physical device's queue family indices

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, queueFamilies.data());

    bool minQueueFamilyFound = false;
    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Transfer)] = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Compute)] = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            minQueueFamilyFound = true;
            mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Graphics)] = i;
        }

        VkBool32 presentSupport = false;
        if (mSurfaceKHR != nullptr)
            vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, i, mSurfaceKHR, &presentSupport);

        if (presentSupport)
            mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Present)] = i;

        i++;
    }

    GRACE_ASSERT_MSG(minQueueFamilyFound, "Minimum queue family required (Graphics) not found!");

    // Create a queue for each family
    queueCreateInfos.reserve(static_cast<uint32_t>(QueueFamily::Undefined));

    std::set<uint32_t> uniqueQueueFamilies = {
        mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Transfer)].value(),
        mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Compute)].value(),
        mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Graphics)].value(),
    };

    if (mSurfaceKHR != nullptr)
        uniqueQueueFamilies.emplace(mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Present)].value());

    // Queue priorities are floats in [0.0, 1.0] - required
    const float queuePriority = 1.0F;
    for (const uint32_t queueFamily : uniqueQueueFamilies)
    {
        const VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };
        queueCreateInfos.push_back(queueCreateInfo);
    }
}

bool Device::IsDeviceSuitable(VkPhysicalDevice device, const std::vector<const char*>& requiredExt) const
{
    GRACE_ASSERT(device != nullptr);

    QueueFamilyIndices indices = FindQueueFamilies(device, mSurfaceKHR);

    bool bExtensionsSupported = CheckDeviceExtensionSupport(device, requiredExt);

    bool bSwapChainAdequate = true;
    if (mSurfaceKHR != nullptr)
    {
        if (bExtensionsSupported)
        {
            SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, mSurfaceKHR);
            bSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty()
                              && indices.presentFamily.has_value();
        }
    }

    VkPhysicalDeviceFeatures2 supportedFeatures = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    vkGetPhysicalDeviceFeatures2(device, &supportedFeatures);

    return indices.graphicsFamily.has_value() && bSwapChainAdequate && bExtensionsSupported;
}

bool Device::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& requiredExt) const
{
    GRACE_ASSERT(device != nullptr);

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

template <GpuManaged T>
GRACE_NODISCARD GpuHandle<T> Device::Create(const GpuObjectDesc<T>& desc, const GpuHandleFlags flags)
{
    const bool disableRefCount = EnumBitmaskHasBitSet(flags, GpuHandleFlags::DisableRefCount);
    GpuHandle<T> newHandle = mGpuObjManager->Create<T>(this, desc, !disableRefCount);

    if constexpr (GpuBindlessCompatible<T>)
    {
        T& t = mGpuObjManager->Get<T>(newHandle);
        if constexpr (std::is_same_v<T, Image>)
        {
            mResourceTable->SubmitImage(t);
        }
        else if constexpr (std::is_same_v<T, Sampler>)
        {
            mResourceTable->SubmitSampler(t);
        }
    }

    return newHandle;
}

template <GpuManaged T>
GRACE_NODISCARD T& Device::Get(const GpuHandle<T>& handle)
{
    return mGpuObjManager->Get<T>(handle);
}

template <GpuManaged T>
void Device::Free(GpuHandle<T>& handle)
{
    mGpuObjManager->Free<T>(handle);
}

template <GpuManaged T>
void Device::FreeDeferred(GpuHandle<T>& handle)
{
    mGpuObjManager->Free<T>(handle, mFrameInFlightIndex);
}

template GRACE_API BufferHandle Device::Create(const GpuBufferDesc&, GpuHandleFlags);
template GRACE_API ImageHandle Device::Create(const GpuImageDesc&, GpuHandleFlags);
template GRACE_API SamplerHandle Device::Create(const GpuSamplerDesc&, GpuHandleFlags);
template GRACE_API PipelineLayoutHandle Device::Create(const GpuPipelineLayoutDesc&, GpuHandleFlags);
template GRACE_API ComputePipelineHandle Device::Create(const GpuComputePipelineDesc&, GpuHandleFlags);
template GRACE_API GraphicsPipelineHandle Device::Create(const GpuGraphicsPipelineDesc&, GpuHandleFlags);
template GRACE_API FenceHandle Device::Create(const GpuFenceDesc&, GpuHandleFlags);
template GRACE_API BinarySemaphoreHandle Device::Create(const GpuBinarySemaphoreDesc&, GpuHandleFlags);
template GRACE_API TimelineSemaphoreHandle Device::Create(const GpuTimelineSemaphoreDesc&, GpuHandleFlags);

template GRACE_API Buffer& Device::Get(const BufferHandle&);
template GRACE_API Image& Device::Get(const ImageHandle&);
template GRACE_API Sampler& Device::Get(const SamplerHandle&);
template GRACE_API PipelineLayout& Device::Get(const PipelineLayoutHandle&);
template GRACE_API ComputePipeline& Device::Get(const ComputePipelineHandle&);
template GRACE_API GraphicsPipeline& Device::Get(const GraphicsPipelineHandle&);
template GRACE_API Fence& Device::Get(const FenceHandle&);
template GRACE_API BinarySemaphore& Device::Get(const BinarySemaphoreHandle&);
template GRACE_API TimelineSemaphore& Device::Get(const TimelineSemaphoreHandle&);

template GRACE_API void Device::Free(BufferHandle&);
template GRACE_API void Device::Free(ImageHandle&);
template GRACE_API void Device::Free(SamplerHandle&);
template GRACE_API void Device::Free(PipelineLayoutHandle&);
template GRACE_API void Device::Free(ComputePipelineHandle&);
template GRACE_API void Device::Free(GraphicsPipelineHandle&);
template GRACE_API void Device::Free(FenceHandle&);
template GRACE_API void Device::Free(BinarySemaphoreHandle&);
template GRACE_API void Device::Free(TimelineSemaphoreHandle&);

template GRACE_API void Device::FreeDeferred(BufferHandle&);
template GRACE_API void Device::FreeDeferred(ImageHandle&);
template GRACE_API void Device::FreeDeferred(SamplerHandle&);
template GRACE_API void Device::FreeDeferred(PipelineLayoutHandle&);
template GRACE_API void Device::FreeDeferred(ComputePipelineHandle&);
template GRACE_API void Device::FreeDeferred(GraphicsPipelineHandle&);
template GRACE_API void Device::FreeDeferred(FenceHandle&);
template GRACE_API void Device::FreeDeferred(BinarySemaphoreHandle&);
template GRACE_API void Device::FreeDeferred(TimelineSemaphoreHandle&);

} // namespace Grace
