#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <Grace/Buffer.hpp>
#include <Grace/Image.hpp>
#include <Grace/Sampler.hpp>
#include <Grace/PipelineGroup.hpp>
#include <Grace/Fence.hpp>
#include <Grace/Semaphore.hpp>
#include <Grace/QueryManager.hpp>
#include <Grace/Swapchain.hpp>
#include <Grace/GraceApi.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/Macros.hpp>

#include <array>

namespace Grace
{

class CommandPool;
class CommandBuffer;
class CommandGroupAllocator;

class ResourceManager;
class GpuResourceTable;

class GRACE_API Device
{
public:
    ~Device();
    Device();
    Device(VkInstance instance, VkSurfaceKHR surface);

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Device&&) noexcept = delete;
    Device& operator=(Device&&) noexcept = delete;

    GRACE_NODISCARD bool IsNull() const;

    GRACE_NODISCARD VkDevice GetVkHandle() const;

    GRACE_NODISCARD VmaAllocator GetVmaHandle() const;

    void WaitIdle();

    GRACE_NODISCARD uint32_t GetCurrentFrameInFlightIndex() const;

    void AdvanceToNextFrame();

    GRACE_NODISCARD VkDescriptorPool& GetSoleDescriptorPool();

    GRACE_NODISCARD VkDescriptorSet& GetSoleDescriptorSet();

    GRACE_NODISCARD VkDescriptorSetLayout& GetSoleDescriptorSetLayout();

    GRACE_NODISCARD PipelineLayoutHandle GetSolePipelineLayout();

    void UpdateBindlessDescriptorSet();

    void CopyMemoryToHostVisibleBuffer(BufferHandle dst,
                                       VkDeviceSize offsetIntoDst,
                                       const void* pHostMem,
                                       VkDeviceSize hostMemBytes);

    void CopyMemoryToHostVisibleImage(ImageHandle dst,
                                      VkDeviceSize offsetIntoDst,
                                      const void* pHostMem,
                                      VkDeviceSize hostMemBytes);

    /// BUFFER OPS

    GRACE_NODISCARD BufferHandle CreateBuffer(const BufferDesc& desc);

    GRACE_NODISCARD Buffer& GetBuffer(const BufferHandle& handle);

    void FreeBuffer(BufferHandle& handle);

    void FreeBufferDeferred(BufferHandle& handle);

    /// IMAGE OPS

    void SubmitImageView(ImageView& view);

    GRACE_NODISCARD ImageHandle CreateImage(const ImageDesc& desc);

    GRACE_NODISCARD ImageHandle CreateSwapchainImage(VkImage image, const ImageDesc& desc);

    GRACE_NODISCARD Image& GetImage(const ImageHandle& handle);

    void FreeImage(ImageHandle& handle);

    void FreeImageDeferred(ImageHandle& handle);

    /// SAMPLER OPS

    GRACE_NODISCARD SamplerHandle CreateSampler(const SamplerDesc& desc);

    GRACE_NODISCARD Sampler& GetSampler(const SamplerHandle& handle);

    void FreeSampler(SamplerHandle& handle);

    void FreeSamplerDeferred(SamplerHandle& handle);

    /// PIPELINE OPS

    GRACE_NODISCARD PipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc&& desc);

    GRACE_NODISCARD PipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc);

    GRACE_NODISCARD Pipeline& GetPipeline(const PipelineHandle& handle);

    void FreePipeline(PipelineHandle& handle);

    GRACE_NODISCARD PipelineLayoutHandle CreatePipelineLayout(const PipelineLayoutDesc& desc);

    GRACE_NODISCARD PipelineLayout& GetPipelineLayout(const PipelineLayoutHandle& handle);

    void FreePipelineLayout(PipelineLayoutHandle& handle);

    /// SYNC OPS

    void WaitForFence(FenceHandle fence, uint64_t timeout = std::numeric_limits<uint64_t>::max());

    void WaitForFences(const std::initializer_list<VkFence>&& fences,
                       uint64_t timeout = std::numeric_limits<uint64_t>::max(),
                       bool waitAll = true);

    void ResetFence(FenceHandle fence);

    void ResetFences(const std::initializer_list<VkFence>&& fences);

    GRACE_NODISCARD FenceHandle CreateFence(const FenceDesc& desc);

    GRACE_NODISCARD Fence& GetFence(const FenceHandle& handle);

    void FreeFence(FenceHandle& handle);

    GRACE_NODISCARD BinarySemaphoreHandle CreateBinarySemaphore(const SemaphoreDesc& desc);

    GRACE_NODISCARD BinarySemaphore& GetBinarySemaphore(const BinarySemaphoreHandle& handle);

    void FreeBinarySemaphore(BinarySemaphoreHandle& handle);

    GRACE_NODISCARD TimelineSemaphoreHandle CreateTimelineSemaphore(const SemaphoreDesc& desc);

    GRACE_NODISCARD TimelineSemaphore& GetTimelineSemaphore(const TimelineSemaphoreHandle& handle);

    void FreeTimelineSemaphore(TimelineSemaphoreHandle& handle);

    /// COMMAND GROUP OPS

    GRACE_NODISCARD CommandPool* GetCommandPool(QueueFamily queueFamily, const char* name);

    void FreeCommandBuffer(CommandBuffer commandBuffer);

    GRACE_NODISCARD CommandBuffer& BeginSingleTimeCommands();

    void EndAndSubmitSingleTimeCommands();

    /// QUEUE OPS

    void SubmitAndWait(QueueFamily queueFamily, const CommandBuffer& cmd);

    void Submit(QueueFamily queue, const CommandBuffer& cmd, const FrameSyncGroup& fsg, FenceHandle fence);

    void BatchSubmit(QueueFamily queue,
                     std::initializer_list<CommandBuffer> cmds,
                     std::initializer_list<FrameSyncGroup> fsgs,
                     FenceHandle fence);

    GRACE_NODISCARD SwapchainStatus Present(const FrameSyncGroup& fsg);

    GRACE_NODISCARD uint32_t GetQueueFamilyIndex(QueueFamily queueFamily);

    GRACE_NODISCARD VkQueue GetQueue(QueueFamily queueFamily);

    /// QUERY OPS

    GRACE_NODISCARD QueryManager* GetQueryManagerPtr();

    template <typename T>
    void ResetQueryPoolFullRange(QueryWriteFlags flags);

    template <typename T>
    GRACE_NODISCARD const QueryGroup<T>&
    GetQueryPoolResults(uint32_t firstQuery, uint32_t queryCount, QueryResult flags) const;

    /// SWAPCHAIN OPS

    GRACE_NODISCARD FrameSyncGroup& AcquireNextSwapchainImage(VkExtent2D imageExtent);

    GRACE_NODISCARD ImageHandle GetRecentlyAcquiredSwapchainImage() const;

    GRACE_NODISCARD const FrameSyncGroup& GetRecentImageAcquiredDesc();

    GRACE_NODISCARD const Format& GetSwapchainFormat() const;

    GRACE_NODISCARD SwapchainStatus GetSwapchainStatus() const;

    void CreateSwapchain(VkExtent2D imageExtent, bool vsync = true);

    /// MISC

    GRACE_NODISCARD VkPhysicalDevice GetPhysicalDevice() const;

    GRACE_NODISCARD VkSurfaceKHR GetSurface() const;

private:
    struct LogicalDeviceDesc
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::vector<const char*> requiredExt;
    };

    void ConfigurePhysicalDevice(VkInstance instance, const std::vector<const char*>& requiredExt);

    void ConfigureLogicalDevice(const LogicalDeviceDesc& desc);

    void ConfigureQueues(std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos);

    GRACE_NODISCARD bool IsDeviceSuitable(VkPhysicalDevice device, const std::vector<const char*>& requiredExt) const;

    GRACE_NODISCARD bool CheckDeviceExtensionSupport(VkPhysicalDevice device,
                                                     const std::vector<const char*>& requiredExt) const;

private:
    VkInstance mParentInstance = nullptr;
    VkDevice mDevice = nullptr;
    VmaAllocator mAllocator = nullptr;
    VkPhysicalDevice mPhysicalDevice = nullptr;
    VkSurfaceKHR mSurfaceKHR = nullptr;
    std::array<std::optional<uint32_t>, static_cast<uint32_t>(QueueFamily::Undefined)> mQueueFamilyIndices = {};
    std::array<VkQueue, static_cast<uint32_t>(QueueFamily::Undefined)> mQueues = {};

    CommandPool* mSingleTimeCmdsPool = nullptr;
    std::unique_ptr<CommandBuffer> mSingleTimeCmdsBuffer = {};
    std::unique_ptr<CommandGroupAllocator> mCmdGroupAllocator = {};
    std::unique_ptr<Swapchain> mSwapchain = {};
    std::unique_ptr<QueryManager> mQueryMgr = {};
    std::unique_ptr<ResourceManager> mResourceMgr = {};
    std::unique_ptr<GpuResourceTable> mResourceTable = {};

    uint32_t mFramesInFlight = 1U;
    uint32_t mFrameInFlightIndex = 0U;
};

template <typename T>
void Device::ResetQueryPoolFullRange(QueryWriteFlags flags)
{
    QueryGroup<T>& qg = mQueryMgr->GetQueryGroup<T>();

    uint32_t first = mFrameInFlightIndex * qg.GetRange();
    uint32_t count = qg.GetRange();

    vkResetQueryPool(mDevice, qg.GetVkQueryPool(), first, count);
    mQueryMgr->ResetQueryGroup<T>();
}

template <typename T>
const QueryGroup<T>& Device::GetQueryPoolResults(uint32_t firstQuery, uint32_t queryCount, QueryResult flags) const
{
    QueryGroup<T>& qg = mQueryMgr->GetQueryGroup<T>();

    uint32_t qc = qg.GetQueryCount();
    uint32_t stride = qg.GetValuesPerQuery() * sizeof(uint64_t);

    if (EnumBitmaskHasBitSet(flags, QueryResult::WithAvailability))
    {
        stride += sizeof(uint64_t);
    }

    uint32_t dataSize = qc * stride;
    VkResult res = vkGetQueryPoolResults(mDevice,
                                         qg.GetVkQueryPool(),
                                         mFrameInFlightIndex * qg.GetRange(),
                                         qc,
                                         dataSize,
                                         qg.GetQueries().data(),
                                         stride,
                                         VK_QUERY_RESULT_64_BIT | static_cast<VkQueryResultFlagBits>(flags));

    qg.mLastResult = res;
    DebugReporter::Check(res);

    return qg;
}

} // namespace Grace
