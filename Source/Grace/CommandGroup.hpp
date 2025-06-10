#pragma once

#include <array>
#include <queue>

#include <vulkan/vulkan.h>
#include <Grace/PipelineGroup.hpp>
#include <Grace/Image.hpp>
#include <Grace/Buffer.hpp>
#include <Grace/Types.hpp>
#include <Grace/SyncGroup.hpp>

namespace Grace
{

class Device;

constexpr uint32_t NUM_QUEUE_TYPES = 4U;

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
    Invalid
};

class CommandBuffer;

class CommandPool
{
public:
    CommandPool(VkCommandPool commandPool, QueueFamily queueFamily);
    ~CommandPool() = default;

    void DestroyIfNotNull(VkDevice device);

    void Reset(Device* device);

    [[nodiscard]] CommandBuffer GetOrAllocateCommandBuffer(Device* device);

    [[nodiscard]] QueueFamily GetQueueFamily() const;

    [[nodiscard]] VkCommandPool GetVkCommandPool() const;

private:
    VkCommandPool m_CommandPool = {};
    QueueFamily m_QueueFamily = QueueFamily::Invalid;
    std::vector<VkCommandBuffer> m_CommandBuffers = {};
    uint32_t m_CommandBuffersInUse = 0;
};

class CommandBuffer
{
public:
    explicit CommandBuffer(VkCommandBuffer cmdBuffer);
    CommandBuffer() = default;

    void Set(VkCommandBuffer cmdBuffer);

    [[nodiscard]] VkCommandBuffer GetVkCommandBuffer() const;

    [[nodiscard]] bool IsNull() const;

    /// UNIVERSAL OPS

    void Reset(VkCommandBufferResetFlags resetFlags = 0) const;

    void BeginRecording(VkCommandBufferUsageFlags usageFlags = 0,
                        const VkCommandBufferInheritanceInfo* pInheritanceInfo = nullptr) const;

    void EndRecording() const;

    void CmdBindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,
                               VkPipelineLayout layout,
                               uint32_t firstSet,
                               const std::vector<VkDescriptorSet>& descriptorSets) const;

    void CmdPushConstants(VkPipelineLayout pipelineLayout, uint32_t size, const void* data) const;

    /// SYNC OPS

    void CmdAddImageLayoutTransition(const Image& image,
                                     ThsvsAccessType prevAccess,
                                     ThsvsAccessType nextAccess,
                                     VkImageAspectFlags aspectMask);

    void CmdAddMemoryBarrier(ThsvsAccessType prevAccess, ThsvsAccessType nextAccess);

    void CmdExecuteBarriers();

    /// GRAPHICS OPS

    void CmdBeginDynamicRendering(const DynamicRenderingDesc& desc) const;

    void CmdEndDynamicRendering() const;

    void CmdSetViewport(const std::vector<VkViewport>& viewports, uint32_t firstViewport = 0) const;

    void CmdSetScissor(const std::vector<VkRect2D>& scissors, uint32_t firstScissor = 0) const;

    void CmdBindGraphicsPipeline(const Pipeline& pipeline) const;

    void CmdBindIndexBuffer(const Buffer& buffer, VkDeviceSize offset, VkIndexType indexType) const;

    void CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const;

    void CmdDrawIndexed(uint32_t indexCount,
                        uint32_t instanceCount,
                        uint32_t firstIndex,
                        int32_t vertexOffset,
                        uint32_t firstInstance) const;

    void CmdDrawIndexedIndirect(const Buffer& buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) const;

    /// COMPUTE OPS

    void CmdBindComputePipeline(const Pipeline& pipeline) const;

    void CmdDispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1) const;

    void CmdDispatchIndirect(const Buffer& buffer, uint64_t offset) const;

    /// TRANSFER OPS

    void CmdBlitImage(const VkBlitImageInfo2& blitInfo) const;

    void CmdClearColorImage(const Image& image,
                            const VkClearColorValue* pColor,
                            const std::vector<VkImageSubresourceRange>& ranges) const;

    void CmdCopyBufferToImage(const Buffer& buffer,
                              const Image& image,
                              VkImageLayout dstLayout,
                              const std::vector<VkBufferImageCopy>& regions) const;

    void
    CmdCopyBuffer(const Buffer& srcBuffer, const Buffer& dstBuffer, const std::vector<VkBufferCopy>& regions) const;

    void CmdFillBuffer(const Buffer& buffer,
                       uint32_t data,
                       VkDeviceSize offset = 0,
                       VkDeviceSize size = VK_WHOLE_SIZE) const;

private:
    VkCommandBuffer m_CmdBuffer = {};
    BarrierBuilder m_BarrierBuilder = {};
};

class CommandGroupAllocator
{
public:
    void Initialise(Device* device);

    void FreeAllRemaining();

    CommandPool* GetOrAllocateCommandPool(QueueFamily queueFamily, const char* name);

    void ReturnCommandPool(CommandPool* commandPool);

    void FreeCommandPool();

    void FreeCommandBuffer();

private:
    Device* m_Device = nullptr;
    // Use std::deque here to prevent any pointer invalidations. No performance hit since
    // new CommandPools are strictly inserted/removed from either end
    std::array<std::deque<CommandPool>, NUM_QUEUE_TYPES> m_AllCommandPoolsAllocated = {};
    std::queue<CommandPool*> m_FreeCommandPools = {};
};

} // namespace Grace
