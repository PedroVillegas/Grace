#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <Grace/ResourceManager.hpp>
#include <Grace/GpuResourceTable.hpp>
#include <Grace/CommandGroup.hpp>
#include <Grace/Swapchain.hpp>
#include <Grace/GraceExport.h>

struct GLFWwindow;

namespace Grace
{

struct GRACE_EXPORT DeviceDesc
{
    uint32_t maxImageDescriptors = 65535U;
    uint32_t maxSamplerDescriptors = 65535U;
    uint32_t maxBufferDescriptors = 65535U;
    bool enableVsync = true;
    bool enableValidationLayers = true;
    GLFWwindow* pGlfwWindow = nullptr;
};

class GRACE_EXPORT Device
{
public:
    ~Device();
    Device() = default;
    Device(VkInstance instance, const DeviceDesc& desc);

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Device&&) noexcept = delete;
    Device& operator=(Device&&) noexcept = delete;

    [[nodiscard]] bool IsNull() const;

    [[nodiscard]] VkDevice GetVkHandle() const;

    [[nodiscard]] VmaAllocator GetVmaHandle() const;

    void WaitIdle();

    void WaitForFences(const std::vector<VkFence>& fences,
                       uint64_t timeout = std::numeric_limits<uint64_t>::max(),
                       bool waitAll = true);

    void ResetFences(const std::vector<VkFence>& fences);

    void Submit(QueueFamily queue,
                const std::vector<CommandBuffer>& cmds,
                const std::vector<VkSemaphore>& waitOn,
                const std::vector<VkSemaphore>& toSignal,
                VkFence fence = {});

    [[nodiscard]] VkDescriptorPool& GetSoleDescriptorPool();

    [[nodiscard]] VkDescriptorSet& GetSoleDescriptorSet();

    [[nodiscard]] VkDescriptorSetLayout& GetSoleDescriptorSetLayout();

    [[nodiscard]] VkPipelineLayout& GetSolePipelineLayout();

    void UpdateBindlessDescriptorSet();

    /// BUFFER OPS

    BufferHandle CreateBuffer(const BufferDesc& desc);

    Buffer& GetBuffer(const BufferHandle& handle);

    void FreeBuffer(BufferHandle& handle);

    /// IMAGE OPS

    void SubmitImageView(ImageView& view);

    ImageHandle CreateImage(const ImageDesc& desc);

    Image& GetImage(const ImageHandle& handle);

    std::vector<RegistryEntry<Image>>& GetAllImages();

    void FreeImage(ImageHandle& handle);

    /// SAMPLER OPS

    SamplerHandle CreateSampler(const SamplerDesc& desc);

    Sampler& GetSampler(const SamplerHandle& handle);

    void FreeSampler(SamplerHandle& handle);

    /// PIPELINE OPS

    PipelineHandle CreatePipeline(const PipelineDesc& info);

    Pipeline& GetPipeline(const PipelineHandle& handle);

    void FreePipeline(PipelineHandle& handle);

    PipelineLayoutHandle CreatePipelineLayout(const PipelineLayoutDesc& desc);

    PipelineLayout& GetPipelineLayout(const PipelineLayoutHandle& handle);

    void FreePipelineLayout(PipelineLayoutHandle& handle);

    /// COMMAND GROUP OPS

    CommandPool* GetCommandPool(QueueFamily queueFamily, const char* name);

    void FreeCommandBuffer(CommandBuffer commandBuffer);

    /// QUEUE OPS

    SwapchainStatus Present(VkSemaphore waitSemaphore, uint32_t swapchainImageIndex);

    uint32_t GetQueueFamilyIndex(QueueFamily queueFamily);

    VkQueue GetQueue(QueueFamily queueFamily);

    /// SWAPCHAIN OPS

    FrameSyncGroup& AcquireNextSwapchainImage(VkExtent2D imageExtent);

    const FrameSyncGroup& GetRecentImageAcquiredDesc();

    void CreateSwapchain(VkExtent2D imageExtent);

    Swapchain& GetSwapchain() const;

    SwapchainStatus GetSwapchainStatus() const;

    /// MISC

    VkPhysicalDevice GetPhysicalDevice() const;

    VkSurfaceKHR GetSurface() const;

private:
    struct LogicalDeviceDesc
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::vector<const char*> requiredExt;
        std::vector<const char*> validationLayers;
    };

    void ConfigurePhysicalDevice(VkInstance instance, const std::vector<const char*>& requiredExt);

    void ConfigureLogicalDevice(const LogicalDeviceDesc& desc);

    void ConfigureQueues(std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos);

    [[nodiscard]] bool IsDeviceSuitable(VkPhysicalDevice device, const std::vector<const char*>& requiredExt) const;

    [[nodiscard]] bool CheckDeviceExtensionSupport(VkPhysicalDevice device,
                                                   const std::vector<const char*>& requiredExt) const;

private:
    VkInstance m_ParentInstance = {};
    VkDevice m_Device = {};
    VmaAllocator m_Allocator = {};
    VkPhysicalDevice m_PhysicalDevice = {};
    VkSurfaceKHR m_SurfaceKHR = {};
    std::unique_ptr<Swapchain> m_Swapchain = {};
    std::array<std::optional<uint32_t>, NUM_QUEUE_TYPES> m_QueueFamilyIndices = {};
    std::array<VkQueue, NUM_QUEUE_TYPES> m_Queues = {};

    std::unique_ptr<ResourceManager> m_ResourceMgr = {};
    std::unique_ptr<GpuResourceTable> m_ResourceTable = {};
    std::unique_ptr<CommandGroupAllocator> m_CmdGroupAllocator = {};
};

} // namespace Grace
