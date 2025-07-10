#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <Grace/QueryManager.hpp>
#include <Grace/ResourceManager.hpp>
#include <Grace/GpuResourceTable.hpp>
#include <Grace/CommandGroup.hpp>
#include <Grace/Swapchain.hpp>
#include <Grace/GraceExport.h>
#include <Grace/DebugReporter.hpp>
#include <Grace/Macros.hpp>

struct GLFWwindow;

namespace Grace
{

struct GRACE_EXPORT DeviceDesc
{
    uint32_t maxImageDescriptors = 65535U;
    uint32_t maxSamplerDescriptors = 65535U;
    uint32_t maxBufferDescriptors = 65535U;
    uint32_t framesInFlight = 1U;
    QueryGroupDesc queryGroupDesc = {};
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

    _NODISCARD bool IsNull() const;

    _NODISCARD VkDevice GetVkHandle() const;

    _NODISCARD VmaAllocator GetVmaHandle() const;

    void WaitIdle();

    void WaitForFence(const Fence& fence, uint64_t timeout = std::numeric_limits<uint64_t>::max());

    void WaitForFences(const std::vector<VkFence>& fences,
                       uint64_t timeout = std::numeric_limits<uint64_t>::max(),
                       bool waitAll = true);

    void ResetFence(const Fence& fence);

    void ResetFences(const std::vector<VkFence>& fences);

    void Submit(QueueFamily queue,
                const std::vector<CommandBuffer>& cmds,
                const std::vector<VkSemaphore>& waitOn,
                const std::vector<VkSemaphore>& toSignal,
                const Fence& fence = {});

    _NODISCARD VkDescriptorPool& GetSoleDescriptorPool();

    _NODISCARD VkDescriptorSet& GetSoleDescriptorSet();

    _NODISCARD VkDescriptorSetLayout& GetSoleDescriptorSetLayout();

    _NODISCARD VkPipelineLayout& GetSolePipelineLayout();

    void UpdateBindlessDescriptorSet();

    /// BUFFER OPS

    _NODISCARD BufferHandle CreateBuffer(const BufferDesc& desc);

    _NODISCARD Buffer& GetBuffer(const BufferHandle& handle);

    void FreeBuffer(BufferHandle& handle);

    /// IMAGE OPS

    void SubmitImageView(ImageView& view);

    _NODISCARD ImageHandle CreateImage(const ImageDesc& desc);

    _NODISCARD Image& GetImage(const ImageHandle& handle);

    _NODISCARD std::vector<RegistryEntry<Image>>& GetAllImages();

    void FreeImage(ImageHandle& handle);

    /// SAMPLER OPS

    _NODISCARD SamplerHandle CreateSampler(const SamplerDesc& desc);

    _NODISCARD Sampler& GetSampler(const SamplerHandle& handle);

    void FreeSampler(SamplerHandle& handle);

    /// PIPELINE OPS

    _NODISCARD PipelineHandle CreatePipeline(const PipelineDesc& info);

    _NODISCARD Pipeline& GetPipeline(const PipelineHandle& handle);

    void FreePipeline(PipelineHandle& handle);

    _NODISCARD PipelineLayoutHandle CreatePipelineLayout(const PipelineLayoutDesc& desc);

    _NODISCARD PipelineLayout& GetPipelineLayout(const PipelineLayoutHandle& handle);

    void FreePipelineLayout(PipelineLayoutHandle& handle);

    /// SYNC OPS

    _NODISCARD FenceHandle CreateFence(const FenceDesc& desc);

    _NODISCARD Fence& GetFence(const FenceHandle& handle);

    void FreeFence(FenceHandle& handle);

    /// COMMAND GROUP OPS

    _NODISCARD CommandPool* GetCommandPool(QueueFamily queueFamily, const char* name);

    void FreeCommandBuffer(CommandBuffer commandBuffer);

    /// QUEUE OPS

    _NODISCARD SwapchainStatus Present(VkSemaphore waitSemaphore, uint32_t swapchainImageIndex);

    _NODISCARD uint32_t GetQueueFamilyIndex(QueueFamily queueFamily);

    _NODISCARD VkQueue GetQueue(QueueFamily queueFamily);

    /// QUERY OPS

    _NODISCARD QueryManager* GetQueryManagerPtr();

    template <typename T>
    void ResetQueryPoolFullRange(uint32_t frameIndex, QueryWriteFlags flags)
    {
        QueryGroup<T>& qg = m_QueryMgr->GetQueryGroup<T>();

        uint32_t first = frameIndex * qg.GetRange();
        uint32_t count = qg.GetRange();

        vkResetQueryPool(m_Device, qg.GetVkQueryPool(), first, count);
        m_QueryMgr->ResetQueryGroup<T>();
    }

    template <typename T>
    _NODISCARD const QueryGroup<T>& GetQueryPoolResults(uint32_t firstQuery,
                                                        uint32_t queryCount,
                                                        VkQueryResultFlags flags,
                                                        uint32_t frameIndex = 0U) const
    {
        QueryGroup<T>& qg = m_QueryMgr->GetQueryGroup<T>();

        uint32_t qc = qg.GetQueryCount();
        uint32_t stride = qg.GetValuesPerQuery() * sizeof(uint64_t);

        if ((flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) != 0)
        {
            stride += sizeof(uint64_t);
        }

        uint32_t dataSize = qc * stride;
        DebugReporter::Check(vkGetQueryPoolResults(m_Device,
                                                   qg.GetVkQueryPool(),
                                                   frameIndex * qg.GetRange(),
                                                   qc,
                                                   dataSize,
                                                   qg.GetQueries().data(),
                                                   stride,
                                                   VK_QUERY_RESULT_64_BIT | flags));

        return qg;
    }

    /// SWAPCHAIN OPS

    _NODISCARD FrameSyncGroup& AcquireNextSwapchainImage(VkExtent2D imageExtent);

    _NODISCARD const Image& GetRecentlyAcquiredSwapchainImage() const;

    _NODISCARD const FrameSyncGroup& GetRecentImageAcquiredDesc();

    _NODISCARD VkFormat GetSwapchainFormat() const;

    _NODISCARD SwapchainStatus GetSwapchainStatus() const;

    void CreateSwapchain(VkExtent2D imageExtent, bool vsync = true);

    /// MISC

    _NODISCARD VkPhysicalDevice GetPhysicalDevice() const;

    _NODISCARD VkSurfaceKHR GetSurface() const;

private:
    struct LogicalDeviceDesc
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::vector<const char*> requiredExt;
    };

    void ConfigurePhysicalDevice(VkInstance instance, const std::vector<const char*>& requiredExt);

    void ConfigureLogicalDevice(const LogicalDeviceDesc& desc);

    void ConfigureQueues(std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos);

    _NODISCARD bool IsDeviceSuitable(VkPhysicalDevice device, const std::vector<const char*>& requiredExt) const;

    _NODISCARD bool CheckDeviceExtensionSupport(VkPhysicalDevice device,
                                                const std::vector<const char*>& requiredExt) const;

private:
    VkInstance m_ParentInstance = {};
    VkDevice m_Device = {};
    VmaAllocator m_Allocator = {};
    VkPhysicalDevice m_PhysicalDevice = {};
    VkSurfaceKHR m_SurfaceKHR = {};
    std::array<std::optional<uint32_t>, static_cast<uint32_t>(QueueFamily::Undefined)> m_QueueFamilyIndices = {};
    std::array<VkQueue, static_cast<uint32_t>(QueueFamily::Undefined)> m_Queues = {};

    std::unique_ptr<Swapchain> m_Swapchain = {};
    std::unique_ptr<QueryManager> m_QueryMgr = {};
    std::unique_ptr<ResourceManager> m_ResourceMgr = {};
    std::unique_ptr<GpuResourceTable> m_ResourceTable = {};
    std::unique_ptr<CommandGroupAllocator> m_CmdGroupAllocator = {};
};

} // namespace Grace
