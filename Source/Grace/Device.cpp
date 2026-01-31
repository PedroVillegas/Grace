#include "Device.hpp"

#include <cassert>
#include <set>

#ifdef GRACE_USE_GLFW
#include <GLFW/glfw3.h>
#endif

#include <Grace/Context.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/HelperFunctions.hpp>
#include <Grace/CommandGroup.hpp>

namespace Grace
{

Device::~Device()
{
    if (mSurfaceKHR != VK_NULL_HANDLE)
    {
        mSwapchain.reset();
        vkDestroySurfaceKHR(mParentInstance, mSurfaceKHR, nullptr);
    }

    mResourceMgr.reset();
    mResourceTable.reset();
    mCmdGroupAllocator.reset();
    mQueryMgr.reset();
    vmaDestroyAllocator(mAllocator);
    vkDestroyDevice(mDevice, nullptr);
}

Device::Device() = default;

Device::Device(VkInstance instance, const DeviceDesc& desc)
    : mParentInstance(instance), mFramesInFlight(desc.framesInFlight)
{
    assert(desc.framesInFlight > 0);

    LogicalDeviceDesc ldd = {};
    ldd.requiredExt = std::move(desc.requiredExtensions);

#ifdef GRACE_USE_GLFW
    ldd.requiredExt.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#endif

#ifdef GRACE_USE_GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    DebugReporter::Check(glfwCreateWindowSurface(instance, desc.pGlfwWindow, nullptr, &mSurfaceKHR));
#endif

    // Checking for supported extensions
    uint32_t extensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);

    std::vector<VkExtensionProperties> availableInstanceExtensions(extensionsCount);

    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableInstanceExtensions.data());

    std::vector<const char*> extensions
#ifdef GRACE_USE_GLFW
        (glfwExtensions, glfwExtensions + glfwExtensionCount)
#endif
            ;

    for (auto& availableExt : availableInstanceExtensions)
    {
        if (strcmp(availableExt.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

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
            mPhysicalDevice = device;
            break;
        }
    }

    assert(mPhysicalDevice && "Failed to find a suitable GPU!");

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
#ifdef GRACE_USE_GLFW
    vkGetDeviceQueue(mDevice,
                     mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Present)].value(),
                     0,
                     &mQueues[static_cast<uint32_t>(QueueFamily::Present)]);
#endif

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

    mQueryMgr = std::make_unique<QueryManager>(this, desc.framesInFlight, desc.queryGroupDesc);
    mCmdGroupAllocator = std::make_unique<CommandGroupAllocator>(this);
    mResourceMgr = std::make_unique<ResourceManager>(desc.framesInFlight);
    mResourceTable = std::make_unique<GpuResourceTable>(
        this, desc.maxImageDescriptors, desc.maxSamplerDescriptors, desc.maxBufferDescriptors);

    mSingleTimeCmdsPool = GetCommandPool(QueueFamily::Graphics, "Grace::CommandPool::SingleTimeCommands");
    mSingleTimeCmdsBuffer = std::make_unique<CommandBuffer>(mSingleTimeCmdsPool->GetOrAllocateCommandBuffer());
}

bool Device::IsNull() const
{
    return mDevice == nullptr || mAllocator == nullptr;
}

VkDevice Device::GetVkHandle() const
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
    mFrameInFlightIndex = (mFrameInFlightIndex + 1) % mFramesInFlight;
}

void Device::WaitForFence(FenceHandle fence, uint64_t timeout)
{
    const Fence& waitFor = GetFence(fence);
    vkWaitForFences(mDevice, 1, &waitFor.GetVkFence(), VK_TRUE, timeout);
    mResourceMgr->FlushDeletionQueue(mFrameInFlightIndex);
}

void Device::WaitForFences(const std::initializer_list<VkFence>&& fences, uint64_t timeout, bool waitAll)
{
    vkWaitForFences(mDevice, static_cast<uint32_t>(fences.size()), fences.begin(), waitAll, timeout);
    mResourceMgr->FlushDeletionQueue(mFrameInFlightIndex);
}

void Device::ResetFence(FenceHandle fence)
{
    const Fence& toReset = GetFence(fence);
    vkResetFences(mDevice, 1, &toReset.GetVkFence());
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
        .semaphore = GetBinarySemaphore(fsg.acquireSemaphore).GetVkSemaphore(),
        .value = 1,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .deviceIndex = 0,
    };

    VkSemaphoreSubmitInfo signalSemaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = GetBinarySemaphore(fsg.presentSemaphore).GetVkSemaphore(),
        .value = 1,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .deviceIndex = 0,
    };

    const VkSubmitInfo2 submitInfo = SubmitInfo(&cmdInfo, &signalSemaphoreInfo, &waitSemaphoreInfo);

    const Fence& fenceToSignal = GetFence(fence);
    DebugReporter::Check(vkQueueSubmit2(GetQueue(queue), 1, &submitInfo, fenceToSignal.GetVkFence()));
}

SwapchainStatus Device::Present(const FrameSyncGroup& fsg)
{
    const VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &GetBinarySemaphore(fsg.presentSemaphore).GetVkSemaphore(),
        .swapchainCount = 1,
        .pSwapchains = &mSwapchain->GetVkHandle(),
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
    vmaCopyMemoryToAllocation(mAllocator, pHostMem, GetBuffer(dst).GetAllocation(), offsetIntoDst, hostMemBytes);
}

void Device::CopyMemoryToHostVisibleImage(ImageHandle dst,
                                          VkDeviceSize offsetIntoDst,
                                          const void* pHostMem,
                                          VkDeviceSize hostMemBytes)
{
    vmaCopyMemoryToAllocation(mAllocator, pHostMem, GetImage(dst).GetAllocation(), offsetIntoDst, hostMemBytes);
}

void Device::SubmitImageView(ImageView& view)
{
    mResourceTable->SubmitImageView(view);
}

BufferHandle Device::CreateBuffer(const BufferDesc& desc)
{
    BufferHandle newHandle = mResourceMgr->Create<Buffer>(this, desc);

    if (EnumBitmaskHasBitSet(desc.usage, BufferUsage::UniformBuffer))
    {
        Buffer& b = mResourceMgr->Get<Buffer>(newHandle);
        mResourceTable->SubmitBuffer(b);
    }

    return newHandle;
}

Buffer& Device::GetBuffer(const BufferHandle& handle)
{
    return mResourceMgr->Get<Buffer>(handle);
}

void Device::FreeBuffer(BufferHandle& handle)
{
    mResourceMgr->Free<Buffer>(handle);
}

void Device::FreeBufferDeferred(BufferHandle& handle)
{
    mResourceMgr->Free<Buffer>(handle, mFrameInFlightIndex);
}

ImageHandle Device::CreateImage(const ImageDesc& desc)
{
    ImageHandle newHandle = mResourceMgr->Create<Image>(this, desc);

    Image& t = mResourceMgr->Get<Image>(newHandle);
    mResourceTable->SubmitImage(t);

    return newHandle;
}

ImageHandle Device::CreateSwapchainImage(VkImage image, const ImageDesc& desc)
{
    return mResourceMgr->Create<Image>(this, image, desc);
}

Image& Device::GetImage(const ImageHandle& handle)
{
    return mResourceMgr->Get<Image>(handle);
}

std::vector<RegistryEntry<Image>>& Device::GetAllImages()
{
    return mResourceMgr->GetAllImages();
}

void Device::FreeImage(ImageHandle& handle)
{
    mResourceTable->FreeImage(mResourceMgr->Get<Image>(handle));
    mResourceMgr->Free<Image>(handle);
}

void Device::FreeImageDeferred(ImageHandle& handle)
{
    mResourceTable->FreeImage(mResourceMgr->Get<Image>(handle));
    mResourceMgr->Free<Image>(handle, mFrameInFlightIndex);
}

SamplerHandle Device::CreateSampler(const SamplerDesc& desc)
{
    SamplerHandle newHandle = mResourceMgr->Create<Sampler>(this, desc);

    Sampler& s = mResourceMgr->Get<Sampler>(newHandle);
    mResourceTable->SubmitSampler(s);

    return newHandle;
}

Sampler& Device::GetSampler(const SamplerHandle& handle)
{
    return mResourceMgr->Get<Sampler>(handle);
}

void Device::FreeSampler(SamplerHandle& handle)
{
    mResourceTable->FreeSampler(mResourceMgr->Get<Sampler>(handle));
    mResourceMgr->Free<Sampler>(handle);
}

void Device::FreeSamplerDeferred(SamplerHandle& handle)
{
    mResourceTable->FreeSampler(mResourceMgr->Get<Sampler>(handle));
    mResourceMgr->Free<Sampler>(handle, mFrameInFlightIndex);
}

PipelineHandle Device::CreateComputePipeline(const ComputePipelineDesc& desc)
{
    const PipelineLayout& pl = mResourceMgr->Get<PipelineLayout>(desc.layout);
    return mResourceMgr->Create<Pipeline>(mDevice, pl, desc);
}

Pipeline& Device::GetPipeline(const PipelineHandle& handle)
{
    return mResourceMgr->Get<Pipeline>(handle);
}

void Device::FreePipeline(PipelineHandle& handle)
{
    mResourceMgr->Free<Pipeline>(handle);
}

PipelineLayoutHandle Device::CreatePipelineLayout(const PipelineLayoutDesc& desc)
{
    return mResourceMgr->Create<PipelineLayout>(this, desc);
}

PipelineLayout& Device::GetPipelineLayout(const PipelineLayoutHandle& handle)
{
    return mResourceMgr->Get<PipelineLayout>(handle);
}

void Device::FreePipelineLayout(PipelineLayoutHandle& handle)
{
    mResourceMgr->Free<PipelineLayout>(handle);
}

FenceHandle Device::CreateFence(const FenceDesc& desc)
{
    return mResourceMgr->Create<Fence>(this, desc);
}

Fence& Device::GetFence(const FenceHandle& handle)
{
    return mResourceMgr->Get<Fence>(handle);
}

void Device::FreeFence(FenceHandle& handle)
{
    mResourceMgr->Free<Fence>(handle);
}

BinarySemaphoreHandle Device::CreateBinarySemaphore(const SemaphoreDesc& desc)
{
    return mResourceMgr->Create<BinarySemaphore>(this, desc);
}

BinarySemaphore& Device::GetBinarySemaphore(const BinarySemaphoreHandle& handle)
{
    return mResourceMgr->Get<BinarySemaphore>(handle);
}

void Device::FreeBinarySemaphore(BinarySemaphoreHandle& handle)
{
    mResourceMgr->Free<BinarySemaphore>(handle);
}

TimelineSemaphoreHandle Device::CreateTimelineSemaphore(const SemaphoreDesc& desc)
{
    return mResourceMgr->Create<TimelineSemaphore>(this, desc);
}

TimelineSemaphore& Device::GetTimelineSemaphore(const TimelineSemaphoreHandle& handle)
{
    return mResourceMgr->Get<TimelineSemaphore>(handle);
}

void Device::FreeTimelineSemaphore(TimelineSemaphoreHandle& handle)
{
    mResourceMgr->Free<TimelineSemaphore>(handle);
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
    assert(queueFamily != QueueFamily::Undefined);
    std::optional<uint32_t> queueFamilyIndex = mQueueFamilyIndices[static_cast<uint32_t>(queueFamily)];
    assert(queueFamilyIndex.has_value());
    return queueFamilyIndex.value();
}

VkQueue Device::GetQueue(QueueFamily queueFamily)
{
    assert(queueFamily != QueueFamily::Undefined);
    std::optional<uint32_t> queueFamilyIndex = mQueueFamilyIndices[static_cast<uint32_t>(queueFamily)];
    assert(queueFamilyIndex.has_value());
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

    assert(deviceCount > 0 && "Failed to find GPUs with Vulkan support!");

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

    assert(mPhysicalDevice && "Failed to find a suitable GPU!");
}

void Device::ConfigureLogicalDevice(const LogicalDeviceDesc& desc)
{
    // Specify device features to be used
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = true;

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
    features2.features = deviceFeatures;

    vkGetPhysicalDeviceFeatures2(mPhysicalDevice, &features2);

    // Set up a logical device to interface with the physical device
    // Can create multiple logical devices from the same physical device if there are varying requirements
    const VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features2,
        .flags = 0,
        .queueCreateInfoCount = static_cast<uint32_t>(desc.queueCreateInfos.size()),
        .pQueueCreateInfos = desc.queueCreateInfos.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(desc.requiredExt.size()),
        .ppEnabledExtensionNames = desc.requiredExt.data(),
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

        if (mSurfaceKHR != nullptr)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, i, mSurfaceKHR, &presentSupport);

            if (presentSupport)
            {
                mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Present)] = i;
            }
        }

        i++;
    }

    assert(minQueueFamilyFound && "Minimum queue family required (Graphics) not found!");

    // Create a queue for each family
    queueCreateInfos.reserve(static_cast<uint32_t>(QueueFamily::Undefined));
    std::set<uint32_t> uniqueQueueFamilies = {
        mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Transfer)].value(),
        mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Compute)].value(),
        mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Graphics)].value(),
#ifdef GRACE_USE_GLFW
        mQueueFamilyIndices[static_cast<uint32_t>(QueueFamily::Present)].value()
#endif
    };

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
    assert(device != nullptr);

    QueueFamilyIndices indices = FindQueueFamilies(device, mSurfaceKHR);

    bool bExtensionsSupported = CheckDeviceExtensionSupport(device, requiredExt);

#ifdef GRACE_USE_GLFW
    bool bSwapChainAdequate = false;
    if (bExtensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, mSurfaceKHR);
        bSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
#endif

    VkPhysicalDeviceFeatures2 supportedFeatures = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    vkGetPhysicalDeviceFeatures2(device, &supportedFeatures);

    return indices.IsComplete()
#ifdef GRACE_USE_GLFW
        && bSwapChainAdequate
#endif
        && bExtensionsSupported;
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
