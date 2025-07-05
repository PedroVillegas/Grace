#pragma once

#include "QueryManager.hpp"

#include <array>
#include <queue>

#include <vulkan/vulkan.h>
#include <Grace/PipelineGroup.hpp>
#include <Grace/Image.hpp>
#include <Grace/Buffer.hpp>
#include <Grace/Types.hpp>
#include <Grace/SyncGroup.hpp>
#include <Grace/GraceExport.h>
#include <Grace/Macros.hpp>

namespace Grace
{

class Device;

enum class QueueFamily : uint32_t
{
    /// Queue supporting transfer operations
    Transfer,
    /// Queue supporting transfer and compute pipeline operations
    Compute,
    /// Queue supporting transfer and graphics pipeline operations
    Graphics,
    /// Queue supporting present operations
    Present,
    /// Queue is undefined
    Undefined
};

class CommandBuffer;

class GRACE_EXPORT CommandPool
{
public:
    ~CommandPool();
    CommandPool(Device* pDevice, VkCommandPool commandPool, QueueFamily queueFamily);

    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;

    CommandPool(CommandPool&& other) noexcept;
    CommandPool& operator=(CommandPool&& other) noexcept;

    void Reset();

    _NODISCARD CommandBuffer GetOrAllocateCommandBuffer();

    _NODISCARD QueueFamily GetQueueFamily() const;

    _NODISCARD VkCommandPool GetVkCommandPool() const;

private:
    Device* m_Device = nullptr;
    VkCommandPool m_CommandPool = nullptr;
    QueueFamily m_QueueFamily = QueueFamily::Undefined;
    std::vector<VkCommandBuffer> m_CommandBuffers = {};
    uint32_t m_CommandBuffersInUse = 0;
};

class GRACE_EXPORT CommandBuffer
{
public:
    ~CommandBuffer() = default;
    CommandBuffer() = default;
    CommandBuffer(VkCommandBuffer commandBuffer, QueueFamily queueFamily, QueryManager* pQueryMgr);

    CommandBuffer(const CommandBuffer&) = default;
    CommandBuffer& operator=(const CommandBuffer&) = default;

    CommandBuffer(CommandBuffer&&) noexcept = default;
    CommandBuffer& operator=(CommandBuffer&&) noexcept = default;

    _NODISCARD bool IsNull() const;

    _NODISCARD VkCommandBuffer GetVkCommandBuffer() const;

    /// UNIVERSAL OPS

    void Reset(VkCommandBufferResetFlags resetFlags = 0) const;

    void BeginRecording(VkCommandBufferUsageFlags usageFlags = 0,
                        const VkCommandBufferInheritanceInfo* pInheritanceInfo = nullptr) const;

    void EndRecording() const;

    void BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,
                            VkPipelineLayout layout,
                            uint32_t firstSet,
                            const std::vector<VkDescriptorSet>& descriptorSets) const;

    void PushConstants(VkPipelineLayout pipelineLayout, uint32_t size, const void* data) const;

    void BeginDebugLabel(const char* label, const std::array<float, 4>& color = { 0.6F, 0.6F, 0.6F, 1.0F }) const;

    void InsertDebugLabel(const char* label, const std::array<float, 4>& color = { 0.6F, 0.6F, 0.6F, 1.0F }) const;

    void EndDebugLabel() const;

    /// SYNC OPS

    void AddBufferBarrier(const Buffer& buffer,
                          std::vector<AccessType>&& accessesBefore,
                          std::vector<AccessType>&& accessesAfter);

    void AddImageBarrier(const Image& image,
                         std::vector<AccessType>&& accessesBefore,
                         std::vector<AccessType>&& accessesAfter);

    void AddMemoryBarrier(std::vector<AccessType>&& prevAccesses, std::vector<AccessType>&& nextAccesses);

    void PipelineBarrier();

    /// GRAPHICS OPS

    void BeginDynamicRendering(const DynamicRenderingDesc& desc) const;

    void EndDynamicRendering() const;

    void SetViewport(const std::vector<VkViewport>& viewports, uint32_t firstViewport = 0) const;

    void SetScissor(const std::vector<VkRect2D>& scissors, uint32_t firstScissor = 0) const;

    void BindGraphicsPipeline(const Pipeline& pipeline) const;

    void BindIndexBuffer(const Buffer& buffer, VkDeviceSize offset, VkIndexType indexType) const;

    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const;

    void DrawIndexed(uint32_t indexCount,
                     uint32_t instanceCount,
                     uint32_t firstIndex,
                     int32_t vertexOffset,
                     uint32_t firstInstance) const;

    void DrawIndexedIndirect(const Buffer& buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) const;

    /// COMPUTE OPS

    void BindComputePipeline(const Pipeline& pipeline) const;

    void Dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1) const;

    void DispatchIndirect(const Buffer& buffer, uint64_t offset) const;

    /// TRANSFER OPS

    void BlitImage(const VkBlitImageInfo2& blitInfo) const;

    void ClearColorImage(const Image& image,
                         const VkClearColorValue& color,
                         const std::vector<VkImageSubresourceRange>& ranges) const;

    void CopyBufferToImage(const Buffer& buffer,
                           const Image& image,
                           VkImageLayout dstLayout,
                           const std::vector<VkBufferImageCopy>& regions) const;

    void CopyBuffer(const Buffer& srcBuffer, const Buffer& dstBuffer, const std::vector<VkBufferCopy>& regions) const;

    void
    FillBuffer(const Buffer& buffer, uint32_t data, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE) const;

    /// QUERY OPS

    void ResetQueryPoolFullRange(QueryType qt, uint32_t frameIndex = 0U);

    void ResetQueryPool(QueryType qt, uint32_t firstQuery, uint32_t queryCount, uint32_t frameIndex = 0U);

    void BeginQuery(QueryType qt,
                    const char* name,
                    QueryWriteFlags writeFlags = QueryWriteFlags::None,
                    uint32_t frameIndex = 0U,
                    VkQueryControlFlags controlFlags = 0);

    void EndQuery(QueryType qt, const char* name);

    void WriteTimestamp(const char* name,
                        VkPipelineStageFlags2 stage,
                        QueryWriteFlags flags = QueryWriteFlags::None,
                        uint32_t frameIndex = 0U);

private:
    VkCommandBuffer m_CmdBuffer = {};
    BarrierBuilder m_BarrierBuilder = {};
    QueueFamily m_QueueFamily = QueueFamily::Undefined;
    QueryManager* m_pQueryMgr = nullptr;
};

class GRACE_EXPORT CommandGroupAllocator
{
public:
    ~CommandGroupAllocator() = default;
    CommandGroupAllocator() = default;
    explicit CommandGroupAllocator(Device* device);

    CommandGroupAllocator(const CommandGroupAllocator&) = delete;
    CommandGroupAllocator& operator=(const CommandGroupAllocator&) = delete;

    CommandGroupAllocator(CommandGroupAllocator&&) noexcept = delete;
    CommandGroupAllocator& operator=(CommandGroupAllocator&&) noexcept = delete;

    _NODISCARD CommandPool* GetOrAllocateCommandPool(QueueFamily queueFamily, const char* name);

    void ReturnCommandPool(CommandPool* commandPool);

    void FreeCommandPool();

    void FreeCommandBuffer();

private:
    Device* m_Device = nullptr;
    // Use std::deque here to prevent any pointer invalidations. No performance hit since
    // new CommandPools are strictly inserted/removed from either end
    std::array<std::deque<CommandPool>, static_cast<uint32_t>(QueueFamily::Undefined)> m_AllCommandPoolsAllocated = {};
    std::queue<CommandPool*> m_FreeCommandPools = {};
};

} // namespace Grace
