#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <Grace/QueryManager.hpp>
#include <Grace/ResourceManager.hpp>
#include <Grace/GpuResourceTable.hpp>
#include <Grace/Swapchain.hpp>
#include <Grace/GraceExport.h>
#include <Grace/DebugReporter.hpp>
#include <Grace/Macros.hpp>

struct GLFWwindow;

namespace Grace
{

class CommandPool;
class CommandBuffer;
class CommandGroupAllocator;

struct GRACE_EXPORT DeviceDesc
{
    uint32_t maxImageDescriptors = 65535U;
    uint32_t maxSamplerDescriptors = 65535U;
    uint32_t maxBufferDescriptors = 65535U;
    uint32_t framesInFlight = 1U;
    std::vector<const char*> requiredExtensions = {};
    QueryGroupDesc queryGroupDesc = {};
    GLFWwindow* pGlfwWindow = nullptr;
};

class GRACE_EXPORT Device
{
public:
    ~Device();
    Device();
    Device(VkInstance instance, const DeviceDesc& desc);

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

    GRACE_NODISCARD std::vector<RegistryEntry<Image>>& GetAllImages();

    void FreeImage(ImageHandle& handle);

    void FreeImageDeferred(ImageHandle& handle);

    /// SAMPLER OPS

    GRACE_NODISCARD SamplerHandle CreateSampler(const SamplerDesc& desc);

    GRACE_NODISCARD Sampler& GetSampler(const SamplerHandle& handle);

    void FreeSampler(SamplerHandle& handle);

    void FreeSamplerDeferred(SamplerHandle& handle);

    /// PIPELINE OPS

    GRACE_NODISCARD PipelineHandle CreatePipeline(const PipelineDesc& info);

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

    template <size_t N>
    void BatchSubmit(QueueFamily queue,
                     CommandBuffer (&& cmds)[N],
                     FrameSyncGroup (&& fsgs)[N],
                     FenceHandle fence)
    {
        static_assert(N > 0, "N must be greater than zero");

        std::array<VkSubmitInfo2, N> submitInfos;

        for (uint32_t i = 0; i < N; ++i)
        {
            VkCommandBufferSubmitInfo cmdInfo = {};
            cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
            cmdInfo.pNext = nullptr;
            cmdInfo.commandBuffer = cmds[i].GetVkCommandBuffer();
            cmdInfo.deviceMask = 0;

            VkSemaphoreSubmitInfo waitSemaphoreInfo = {};
            waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            waitSemaphoreInfo.pNext = nullptr;
            waitSemaphoreInfo.semaphore = GetBinarySemaphore(fsgs[i].acquireSemaphore).GetVkSemaphore();
            waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            waitSemaphoreInfo.deviceIndex = 0;
            waitSemaphoreInfo.value = 1;

            VkSemaphoreSubmitInfo signalSemaphoreInfo = {};
            signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            signalSemaphoreInfo.pNext = nullptr;
            signalSemaphoreInfo.semaphore = GetBinarySemaphore(fsgs[i].presentSemaphore).GetVkSemaphore();
            signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            signalSemaphoreInfo.deviceIndex = 0;
            signalSemaphoreInfo.value = 1;

            submitInfos[i] = SubmitInfo(&cmdInfo, &signalSemaphoreInfo, &waitSemaphoreInfo);
        }

        const Fence& fenceToSignal = GetFence(fence);
        DebugReporter::Check(vkQueueSubmit2(GetQueue(queue), N, submitInfos.data(), fenceToSignal.GetVkFence()));
    }

    template <size_t N>
    void BatchSubmit(QueueFamily queue,
                     CommandBuffer (&& cmds)[N],
                     FenceHandle fence)
    {
        static_assert(N > 0, "N must be greater than zero");

        std::array<VkSubmitInfo2, N> submitInfos;

        for (uint32_t i = 0; i < N; ++i)
        {
            VkCommandBufferSubmitInfo cmdInfo = {};
            cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
            cmdInfo.pNext = nullptr;
            cmdInfo.commandBuffer = cmds[i].GetVkCommandBuffer();
            cmdInfo.deviceMask = 0;

            submitInfos[i] = SubmitInfo(&cmdInfo, nullptr, nullptr);
        }

        const Fence& fenceToSignal = GetFence(fence);
        DebugReporter::Check(vkQueueSubmit2(GetQueue(queue), N, submitInfos.data(), fenceToSignal.GetVkFence()));
    }

    GRACE_NODISCARD SwapchainStatus Present(const FrameSyncGroup& fsg);

    GRACE_NODISCARD uint32_t GetQueueFamilyIndex(QueueFamily queueFamily);

    GRACE_NODISCARD VkQueue GetQueue(QueueFamily queueFamily);

    /// QUERY OPS

    GRACE_NODISCARD QueryManager* GetQueryManagerPtr();

    template <typename T>
    void ResetQueryPoolFullRange(QueryWriteFlags flags)
    {
        QueryGroup<T>& qg = m_QueryMgr->GetQueryGroup<T>();

        uint32_t first = m_FrameInFlightIndex * qg.GetRange();
        uint32_t count = qg.GetRange();

        vkResetQueryPool(m_Device, qg.GetVkQueryPool(), first, count);
        m_QueryMgr->ResetQueryGroup<T>();
    }

    template <typename T>
    GRACE_NODISCARD const QueryGroup<T>&
    GetQueryPoolResults(uint32_t firstQuery, uint32_t queryCount, QueryResult flags) const
    {
        QueryGroup<T>& qg = m_QueryMgr->GetQueryGroup<T>();

        uint32_t qc = qg.GetQueryCount();
        uint32_t stride = qg.GetValuesPerQuery() * sizeof(uint64_t);

        if (EnumBitmaskHasBitSet(flags, QueryResult::WithAvailability))
        {
            stride += sizeof(uint64_t);
        }

        uint32_t dataSize = qc * stride;
        VkResult res = vkGetQueryPoolResults(m_Device,
                                             qg.GetVkQueryPool(),
                                             m_FrameInFlightIndex * qg.GetRange(),
                                             qc,
                                             dataSize,
                                             qg.GetQueries().data(),
                                             stride,
                                             VK_QUERY_RESULT_64_BIT | static_cast<VkQueryResultFlagBits>(flags));

        qg.m_LastResult = res;
        DebugReporter::Check(res);

        return qg;
    }

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
    VkInstance m_ParentInstance = {};
    VkDevice m_Device = {};
    VmaAllocator m_Allocator = {};
    VkPhysicalDevice m_PhysicalDevice = {};
    VkSurfaceKHR m_SurfaceKHR = {};
    std::array<std::optional<uint32_t>, static_cast<uint32_t>(QueueFamily::Undefined)> m_QueueFamilyIndices = {};
    std::array<VkQueue, static_cast<uint32_t>(QueueFamily::Undefined)> m_Queues = {};

    CommandPool* m_SingleTimeCmdsPool = nullptr;
    std::unique_ptr<CommandBuffer> m_SingleTimeCmdsBuffer = {};
    std::unique_ptr<CommandGroupAllocator> m_CmdGroupAllocator = {};
    std::unique_ptr<Swapchain> m_Swapchain = {};
    std::unique_ptr<QueryManager> m_QueryMgr = {};
    std::unique_ptr<ResourceManager> m_ResourceMgr = {};
    std::unique_ptr<GpuResourceTable> m_ResourceTable = {};

    uint32_t m_FramesInFlight = 1U;
    uint32_t m_FrameInFlightIndex = 0U;
};

} // namespace Grace
