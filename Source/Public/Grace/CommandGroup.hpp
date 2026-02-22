#pragma once

#include <array>
#include <queue>
#include <cassert>

#include <Grace/PipelineGroup.hpp>
#include <Grace/QueryManager.hpp>
#include <Grace/GpuResourceHandles.hpp>
#include <Grace/TypesVector.hpp>
#include <Grace/Types.hpp>
#include <Grace/GraceApi.hpp>
#include <Grace/Macros.hpp>
#include <Grace/Device.hpp>

namespace Grace
{

class Device;

class GRACE_API CommandBuffer
{
public:
    CommandBuffer() = default;
    CommandBuffer(Device* pDevice, VkCommandBuffer commandBuffer, QueueFamily queueFamily, QueryManager* pQueryMgr);

    GRACE_NODISCARD bool IsNull() const;

    GRACE_NODISCARD const VkCommandBuffer& GetVkCommandBuffer() const;

    /// UNIVERSAL OPS

    void Reset(VkCommandBufferResetFlags resetFlags = 0) const;

    void BeginRecording(VkCommandBufferUsageFlags usageFlags = 0,
                        const VkCommandBufferInheritanceInfo* pInheritanceInfo = nullptr) const;

    void EndRecording() const;

    void BindPipeline(PipelineHandle pipeline) const;

    void BindDescriptorSets(PipelineBindPoint bindpoint,
                            PipelineLayoutHandle layout,
                            const std::initializer_list<VkDescriptorSet>&& descriptorSets) const;

    void PushConstants(PipelineLayoutHandle layout, uint32_t size, const void* data) const;

    template <typename DataStruct>
    void PushConstants(PipelineLayoutHandle layout,
                       const DataStruct* data,
                       ShaderStage stage = ShaderStage::All,
                       uint32_t offset = 0) const
    {
        static_assert(sizeof(DataStruct) <= 128, "DataStruct is too large, must not exceed 128 bytes!");
        const PipelineLayout& pl = mDevicePtr->GetPipelineLayout(layout);
        assert(!pl.IsNull());
        vkCmdPushConstants(mCmdBuffer,
                           pl.GetVkPipelineLayout(),
                           static_cast<VkShaderStageFlags>(stage),
                           offset,
                           sizeof(DataStruct),
                           data);
    }

    void BeginDebugLabel(const char* label, const Float4& colour = Float4(0.6F, 1.0F)) const;

    void InsertDebugLabel(const char* label, const Float4& colour = Float4(0.6F, 1.0F)) const;

    void EndDebugLabel() const;

    /// SYNC OPS

    void AddBufferBarrier(BufferHandle buffer,
                          std::vector<AccessType>&& accessesBefore,
                          std::vector<AccessType>&& accessesAfter);

    void AddImageBarrier(ImageHandle image,
                         std::vector<AccessType>&& accessesBefore,
                         std::vector<AccessType>&& accessesAfter);

    void AddMemoryBarrier(std::vector<AccessType>&& prevAccesses, std::vector<AccessType>&& nextAccesses);

    void PipelineBarrier();

    /// GRAPHICS OPS

    void BeginDynamicRendering(const DynamicRenderingDesc&& desc) const;

    void EndDynamicRendering() const;

    void SetViewport(const std::initializer_list<VkViewport>&& viewports) const;

    void SetScissor(const std::initializer_list<VkRect2D>&& scissors) const;

    void BindIndexBuffer(BufferHandle buffer, VkDeviceSize offset, IndexType indexType) const;

    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const;

    void DrawIndexed(uint32_t indexCount,
                     uint32_t instanceCount,
                     uint32_t firstIndex,
                     int32_t vertexOffset,
                     uint32_t firstInstance) const;

    void DrawIndexedIndirect(BufferHandle buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) const;

    /// COMPUTE OPS

    void Dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1) const;

    void DispatchIndirect(BufferHandle buffer, uint64_t offset) const;

    /// TRANSFER OPS

    void BlitImage(const VkBlitImageInfo2& blitInfo) const;

    void ClearColorImage(ImageHandle image, const ClearColourValue& color) const;

    void ClearColorImageRanges(ImageHandle image,
                               const ClearColourValue& color,
                               const std::initializer_list<VkImageSubresourceRange>&& ranges) const;

    void CopyBufferToImage(BufferHandle buffer, ImageHandle image, VkImageLayout dstLayout) const;

    void CopyBufferToImageRegions(BufferHandle buffer,
                                  ImageHandle image,
                                  VkImageLayout dstLayout,
                                  const std::initializer_list<VkBufferImageCopy>&& regions) const;

    void CopyBuffer(BufferHandle srcBuffer, BufferHandle dstBuffer) const;

    void CopyBufferRanges(BufferHandle srcBuffer,
                          BufferHandle dstBuffer,
                          const std::initializer_list<VkBufferCopy>&& regions) const;

    void
    FillBuffer(BufferHandle buffer, uint32_t data, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE) const;

    /// QUERY OPS

    template <typename T>
    void ResetQueryPoolFullRange(uint32_t frameIndex)
    {
        assert(mQueryMgrPtr);

        const QueryGroup<T>& qg = mQueryMgrPtr->GetQueryGroup<T>();
        mQueryMgrPtr->ResetQueryGroup<T>();

        uint32_t start = frameIndex * qg.GetRange();
        uint32_t end = qg.GetRange();

        vkCmdResetQueryPool(mCmdBuffer, qg.GetVkQueryPool(), start, end);
    }

    template <typename T>
    void ResetQueryPool(uint32_t firstQuery, uint32_t queryCount, uint32_t frameIndex)
    {
        assert(mQueryMgrPtr);

        const QueryGroup<T>& qg = mQueryMgrPtr->GetQueryGroup<T>();
        assert(queryCount < qg.GetRange());

        uint32_t start = (frameIndex * qg.GetRange()) + firstQuery;
        uint32_t end = start + queryCount;

        vkCmdResetQueryPool(mCmdBuffer, qg.GetVkQueryPool(), start, end);
    }

    template <typename T>
    void BeginQuery(const char* name,
                    uint32_t frameIndex,
                    QueryWriteFlags writeFlags = QueryWriteFlags::None,
                    VkQueryControlFlags controlFlags = 0)
    {
        QueryGroup<T>& qg = mQueryMgrPtr->GetQueryGroup<T>();
        const uint32_t query = mQueryMgrPtr->AddQuery<T>(name);

        const uint32_t offset = frameIndex * qg.GetRange();
        if (writeFlags == QueryWriteFlags::WriteIfPreviousResultIsAvailable)
        {
            if (qg.GetQueries()[offset + query + qg.GetValuesPerQuery()] == 0)
            {
                return;
            }
        }

        vkCmdBeginQuery(mCmdBuffer, qg.GetVkQueryPool(), query, controlFlags);
    }

    template <typename T>
    void EndQuery(const char* name)
    {
        const QueryGroup<T>& qg = mQueryMgrPtr->GetQueryGroup<T>();
        vkCmdEndQuery(mCmdBuffer, qg.GetVkQueryPool(), qg.GetQueryOffset(name));
    }

    void WriteTimestamp(const char* name, PipelineStage stage, uint32_t frameIndex) const;

private:
    Device* mDevicePtr = nullptr;
    VkCommandBuffer mCmdBuffer = nullptr;
    QueueFamily mQueueFamily = QueueFamily::Undefined;
    QueryManager* mQueryMgrPtr = nullptr;
};

class GRACE_API CommandPool
{
public:
    ~CommandPool();
    CommandPool(Device* pDevice, VkCommandPool commandPool, QueueFamily queueFamily);

    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;

    CommandPool(CommandPool&& other) noexcept;
    CommandPool& operator=(CommandPool&& other) noexcept;

    void Reset();

    GRACE_NODISCARD CommandBuffer GetOrAllocateCommandBuffer();

    GRACE_NODISCARD QueueFamily GetQueueFamily() const;

    GRACE_NODISCARD VkCommandPool GetVkCommandPool() const;

private:
    Device* mDevice = nullptr;
    VkCommandPool mCommandPool = nullptr;
    QueueFamily mQueueFamily = QueueFamily::Undefined;
    std::vector<VkCommandBuffer> mCommandBuffers = {};
    uint32_t mCommandBuffersInUse = 0;
};

class GRACE_API CommandGroupAllocator
{
public:
    ~CommandGroupAllocator();
    CommandGroupAllocator();
    explicit CommandGroupAllocator(Device* device);

    CommandGroupAllocator(const CommandGroupAllocator&) = delete;
    CommandGroupAllocator& operator=(const CommandGroupAllocator&) = delete;

    CommandGroupAllocator(CommandGroupAllocator&& other) noexcept;
    CommandGroupAllocator& operator=(CommandGroupAllocator&& other) noexcept;

    GRACE_NODISCARD CommandPool* GetOrAllocateCommandPool(QueueFamily queueFamily, const char* name);

    void ReturnCommandPool(CommandPool* commandPool);

    void FreeCommandPool();

    void FreeCommandBuffer();

private:
    Device* mDevicePtr = nullptr;
    // Use std::deque here to prevent any pointer invalidations. No performance hit since
    // new CommandPools are strictly inserted/removed from either end
    std::array<std::deque<CommandPool>, static_cast<uint32_t>(QueueFamily::Undefined)> mAllCommandPoolsAllocated = {};
    std::queue<CommandPool*> mFreeCommandPools = {};
};

} // namespace Grace
