#include "CommandGroup.hpp"

#include <Grace/DebugReporter.hpp>
#include <Grace/Context.hpp>
#include <Grace/HelperFunctions.hpp>

#include <cassert>

namespace Grace
{

CommandGroupAllocator::CommandGroupAllocator(Device* device) : m_Device(device)
{
}

CommandPool* CommandGroupAllocator::GetOrAllocateCommandPool(QueueFamily queueFamily, const char* name)
{
    assert(static_cast<uint32_t>(queueFamily) <= static_cast<uint32_t>(QueueFamily::Graphics));

    // If no pools are free, allocate new pool
    std::deque<CommandPool>& allocatedPoolsOfQueueFamily =
        m_AllCommandPoolsAllocated[static_cast<uint32_t>(queueFamily)];
    if (m_FreeCommandPools.empty())
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.pNext = nullptr;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_Device->GetQueueFamilyIndex(queueFamily);

        VkCommandPool allocatedCmdPool = {};
        DebugReporter::Check(vkCreateCommandPool(m_Device->GetVkHandle(), &poolInfo, nullptr, &allocatedCmdPool));

        allocatedPoolsOfQueueFamily.emplace_back(m_Device, allocatedCmdPool, queueFamily);
        m_FreeCommandPools.push(&allocatedPoolsOfQueueFamily.back());
    }

    assert(!m_FreeCommandPools.empty());
    CommandPool* ret = m_FreeCommandPools.front();
    m_FreeCommandPools.pop();

    AssignDebugName<VkCommandPool>(m_Device->GetVkHandle(), ret->GetVkCommandPool(), name);

    return ret;
}

void CommandGroupAllocator::ReturnCommandPool(CommandPool* commandPool)
{
    m_FreeCommandPools.push(commandPool);
}

void CommandGroupAllocator::FreeCommandPool()
{
}

void CommandGroupAllocator::FreeCommandBuffer()
{
}

CommandPool::~CommandPool()
{
    if (m_CommandPool != nullptr)
    {
        vkDestroyCommandPool(m_Device->GetVkHandle(), m_CommandPool, nullptr);
    }
}

CommandPool::CommandPool(Device* pDevice, VkCommandPool commandPool, QueueFamily queueFamily)
    : m_Device(pDevice), m_CommandPool(commandPool), m_QueueFamily(queueFamily)
{
}

CommandPool::CommandPool(CommandPool&& other) noexcept
    : m_Device(other.m_Device), m_CommandPool(other.m_CommandPool), m_QueueFamily(other.m_QueueFamily),
      m_CommandBuffers(std::move(other.m_CommandBuffers)), m_CommandBuffersInUse(other.m_CommandBuffersInUse)
{
    other.m_CommandPool = nullptr;
}

CommandPool& CommandPool::operator=(CommandPool&& other) noexcept
{
    m_Device = other.m_Device;
    m_CommandPool = other.m_CommandPool;
    m_QueueFamily = other.m_QueueFamily;
    m_CommandBuffers = std::move(other.m_CommandBuffers);
    m_CommandBuffersInUse = other.m_CommandBuffersInUse;
    other.m_CommandPool = nullptr;

    return *this;
}

void CommandPool::Reset()
{
    vkResetCommandPool(m_Device->GetVkHandle(), m_CommandPool, 0);
}

CommandBuffer CommandPool::GetOrAllocateCommandBuffer()
{
    if (m_CommandBuffers.size() == m_CommandBuffersInUse)
    {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer allocated = {};
        DebugReporter::Check(vkAllocateCommandBuffers(m_Device->GetVkHandle(), &allocInfo, &allocated));

        m_CommandBuffers.push_back(allocated);
    }

    return { m_CommandBuffers[m_CommandBuffersInUse++], m_QueueFamily, m_Device->GetQueryManagerPtr() };
}

QueueFamily CommandPool::GetQueueFamily() const
{
    return m_QueueFamily;
}

VkCommandPool CommandPool::GetVkCommandPool() const
{
    return m_CommandPool;
}

CommandBuffer::CommandBuffer(VkCommandBuffer commandBuffer, QueueFamily queueFamily, QueryManager* pQueryMgr)
    : m_CmdBuffer(commandBuffer), m_QueueFamily(queueFamily), m_pQueryMgr(pQueryMgr)
{
}

bool CommandBuffer::IsNull() const
{
    return m_CmdBuffer == nullptr;
}

VkCommandBuffer CommandBuffer::GetVkCommandBuffer() const
{
    return m_CmdBuffer;
}

void CommandBuffer::Reset(VkCommandBufferResetFlags resetFlags) const
{
    DebugReporter::Check(vkResetCommandBuffer(m_CmdBuffer, resetFlags));
}

void CommandBuffer::BeginRecording(VkCommandBufferUsageFlags usageFlags,
                                   const VkCommandBufferInheritanceInfo* pInheritanceInfo) const
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = usageFlags;
    beginInfo.pInheritanceInfo = pInheritanceInfo;

    DebugReporter::Check(vkBeginCommandBuffer(m_CmdBuffer, &beginInfo));
}

void CommandBuffer::EndRecording() const
{
    DebugReporter::Check(vkEndCommandBuffer(m_CmdBuffer));
}

void CommandBuffer::BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,
                                       VkPipelineLayout layout,
                                       uint32_t firstSet,
                                       const std::vector<VkDescriptorSet>& descriptorSets) const
{
    vkCmdBindDescriptorSets(m_CmdBuffer,
                            pipelineBindPoint,
                            layout,
                            firstSet,
                            static_cast<uint32_t>(descriptorSets.size()),
                            descriptorSets.data(),
                            0,
                            nullptr);
}

void CommandBuffer::BeginDynamicRendering(const DynamicRenderingDesc& desc) const
{
    assert(!desc.colorAttachments.empty());
    assert(desc.renderArea.extent.width > 0 && desc.renderArea.extent.height > 0);

    VkRenderingInfo renderInfo = {};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.pNext = nullptr;
    renderInfo.flags = desc.flags;
    renderInfo.renderArea = VkRect2D(desc.renderArea.offset, desc.renderArea.extent);
    renderInfo.layerCount = desc.layerCount;
    renderInfo.viewMask = desc.viewMask;
    renderInfo.colorAttachmentCount = static_cast<uint32_t>(desc.colorAttachments.size());
    renderInfo.pColorAttachments = desc.colorAttachments.data();
    renderInfo.pDepthAttachment = desc.depthAttachments.data();
    renderInfo.pStencilAttachment = desc.stencilAttachments.data();

    vkCmdBeginRendering(m_CmdBuffer, &renderInfo);
}

void CommandBuffer::EndDynamicRendering() const
{
    vkCmdEndRendering(m_CmdBuffer);
}

void CommandBuffer::SetViewport(const std::vector<VkViewport>& viewports, uint32_t firstViewport) const
{
    assert(!viewports.empty());
    vkCmdSetViewport(m_CmdBuffer, firstViewport, static_cast<uint32_t>(viewports.size()), viewports.data());
}

void CommandBuffer::SetScissor(const std::vector<VkRect2D>& scissors, uint32_t firstScissor) const
{
    assert(!scissors.empty());
    vkCmdSetScissor(m_CmdBuffer, firstScissor, static_cast<uint32_t>(scissors.size()), scissors.data());
}

void CommandBuffer::PushConstants(VkPipelineLayout pipelineLayout, uint32_t size, const void* data) const
{
    assert(pipelineLayout != nullptr);
    assert(size <= 128);
    vkCmdPushConstants(m_CmdBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, size, data);
}

void CommandBuffer::BeginDebugLabel(const char* label, const std::array<float, 4>& color) const
{
    VkDebugUtilsLabelEXT labelInfo = {};
    labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    labelInfo.pNext = nullptr;
    labelInfo.pLabelName = label;
    labelInfo.color[0] = color[0];
    labelInfo.color[1] = color[1];
    labelInfo.color[2] = color[2];
    labelInfo.color[3] = color[3];

    vkCmdBeginDebugUtilsLabelEXT_Meta(m_CmdBuffer, &labelInfo);
}

void CommandBuffer::InsertDebugLabel(const char* label, const std::array<float, 4>& color) const
{
    VkDebugUtilsLabelEXT labelInfo = {};
    labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    labelInfo.pNext = nullptr;
    labelInfo.pLabelName = label;
    labelInfo.color[0] = color[0];
    labelInfo.color[1] = color[1];
    labelInfo.color[2] = color[2];
    labelInfo.color[3] = color[3];

    vkCmdInsertDebugUtilsLabelEXT_Meta(m_CmdBuffer, &labelInfo);
}

void CommandBuffer::EndDebugLabel() const
{
    vkCmdEndDebugUtilsLabelEXT_Meta(m_CmdBuffer);
}

void CommandBuffer::AddBufferBarrier(const Buffer& buffer,
                                     std::vector<AccessType>&& accessesBefore,
                                     std::vector<AccessType>&& accessesAfter)
{
    m_BarrierBuilder.AddBufferBarrier(buffer, std::move(accessesBefore), std::move(accessesAfter));
}

void CommandBuffer::AddImageBarrier(const Image& image,
                                    std::vector<AccessType>&& accessesBefore,
                                    std::vector<AccessType>&& accessesAfter)
{
    m_BarrierBuilder.AddImageBarrier(image, std::move(accessesBefore), std::move(accessesAfter));
}

void CommandBuffer::AddMemoryBarrier(std::vector<AccessType>&& accessesBefore, std::vector<AccessType>&& accessesAfter)
{
    m_BarrierBuilder.AddMemoryBarrier(std::move(accessesBefore), std::move(accessesAfter));
}

void CommandBuffer::PipelineBarrier()
{
    m_BarrierBuilder.PipelineBarrier(m_CmdBuffer);
}

void CommandBuffer::BindGraphicsPipeline(const Pipeline& pipeline) const
{
    assert(!pipeline.IsNull());

    vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetVkHandle());
}

void CommandBuffer::BindComputePipeline(const Pipeline& pipeline) const
{
    assert(!pipeline.IsNull());

    vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.GetVkHandle());
}

void CommandBuffer::Dispatch(uint32_t x, uint32_t y, uint32_t z) const
{
    vkCmdDispatch(m_CmdBuffer, x, y, z);
}

void CommandBuffer::DispatchIndirect(const Buffer& buffer, uint64_t offset) const
{
    vkCmdDispatchIndirect(m_CmdBuffer, buffer.GetVkHandle(), offset);
}

void CommandBuffer::BlitImage(const VkBlitImageInfo2& blitInfo) const
{
    vkCmdBlitImage2(m_CmdBuffer, &blitInfo);
}

void CommandBuffer::CopyBufferToImage(const Buffer& buffer,
                                      const Image& image,
                                      VkImageLayout dstLayout,
                                      const std::vector<VkBufferImageCopy>& regions) const
{
    assert(!buffer.IsNull());
    assert(!image.IsNull());
    assert(!regions.empty());
    vkCmdCopyBufferToImage(m_CmdBuffer,
                           buffer.GetVkHandle(),
                           image.GetImage(),
                           dstLayout,
                           static_cast<uint32_t>(regions.size()),
                           regions.data());
}

void CommandBuffer::CopyBuffer(const Buffer& srcBuffer,
                               const Buffer& dstBuffer,
                               const std::vector<VkBufferCopy>& regions) const
{
    assert(!srcBuffer.IsNull());
    assert(!dstBuffer.IsNull());
    assert(!regions.empty());
    vkCmdCopyBuffer(m_CmdBuffer,
                    srcBuffer.GetVkHandle(),
                    dstBuffer.GetVkHandle(),
                    static_cast<uint32_t>(regions.size()),
                    regions.data());
}

void CommandBuffer::FillBuffer(const Buffer& buffer, uint32_t data, VkDeviceSize offset, VkDeviceSize size) const
{
    vkCmdFillBuffer(m_CmdBuffer, buffer.GetVkHandle(), offset, size, data);
}

void CommandBuffer::ResetQueryPoolFullRange(QueryType qt, uint32_t frameIndex)
{
    assert(m_pQueryMgr);
    assert(qt != QueryType::Undefined);

    const QueryGroup& qg = m_pQueryMgr->GetQueryGroup(qt);
    m_pQueryMgr->ResetQueryGroup(qt);

    uint32_t start = frameIndex * qg.GetRange();
    uint32_t end = start + qg.GetRange();

    vkCmdResetQueryPool(m_CmdBuffer, qg.GetVkQueryPool(), start, end);
}

void CommandBuffer::ResetQueryPool(QueryType qt, uint32_t firstQuery, uint32_t queryCount, uint32_t frameIndex)
{
    assert(m_pQueryMgr);
    assert(qt != QueryType::Undefined);

    const QueryGroup& qg = m_pQueryMgr->GetQueryGroup(qt);
    assert(queryCount < qg.GetRange());

    uint32_t start = (frameIndex * qg.GetRange()) + firstQuery;
    uint32_t end = start + queryCount;

    vkCmdResetQueryPool(m_CmdBuffer, qg.GetVkQueryPool(), start, end);
}

void CommandBuffer::BeginQuery(QueryType qt,
                               const char* name,
                               QueryWriteFlags writeFlags,
                               uint32_t frameIndex,
                               VkQueryControlFlags controlFlags)
{
    QueryGroup& qg = m_pQueryMgr->GetQueryGroup(qt);
    const uint32_t query = m_pQueryMgr->AddQuery(qt, name);

    const uint32_t offset = frameIndex * qg.GetRange();
    if (writeFlags == QueryWriteFlags::WriteIfPreviousResultIsAvailable)
    {
        if (qg.GetQueries()[offset + query + qg.GetValuesPerQuery()] == 0)
        {
            return;
        };
    }

    vkCmdBeginQuery(m_CmdBuffer, qg.GetVkQueryPool(), query, controlFlags);
}

void CommandBuffer::EndQuery(QueryType qt, const char* name)
{
    const QueryGroup& qg = m_pQueryMgr->GetQueryGroup(qt);
    vkCmdEndQuery(m_CmdBuffer, qg.GetVkQueryPool(), qg.GetQueryOffset(name));
}

void CommandBuffer::WriteTimestamp(const char* name,
                                   VkPipelineStageFlags2 stage,
                                   QueryWriteFlags flags,
                                   uint32_t frameIndex)
{
    QueryGroup& qg = m_pQueryMgr->GetQueryGroup(QueryType::Timestamp);
    const uint32_t query = m_pQueryMgr->AddQuery(QueryType::Timestamp, name);

    const uint32_t offset = frameIndex * qg.GetRange();
    if (flags == QueryWriteFlags::WriteIfPreviousResultIsAvailable)
    {
        if (qg.GetQueries()[offset + query + 1] == 0)
        {
            return;
        };
    }

    vkCmdWriteTimestamp2(m_CmdBuffer, stage, qg.GetVkQueryPool(), offset + query);
}

void CommandBuffer::ClearColorImage(const Image& image,
                                    const VkClearColorValue& color,
                                    const std::vector<VkImageSubresourceRange>& ranges) const
{
    vkCmdClearColorImage(m_CmdBuffer,
                         image.GetImage(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         &color,
                         static_cast<uint32_t>(ranges.size()),
                         ranges.data());
}

void CommandBuffer::BindIndexBuffer(const Buffer& buffer, VkDeviceSize offset, VkIndexType indexType) const
{
    vkCmdBindIndexBuffer(m_CmdBuffer, buffer.GetVkHandle(), offset, indexType);
}

void CommandBuffer::Draw(uint32_t vertexCount,
                         uint32_t instanceCount,
                         uint32_t firstVertex,
                         uint32_t firstInstance) const
{
    vkCmdDraw(m_CmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::DrawIndexed(uint32_t indexCount,
                                uint32_t instanceCount,
                                uint32_t firstIndex,
                                int32_t vertexOffset,
                                uint32_t firstInstance) const
{
    vkCmdDrawIndexed(m_CmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::DrawIndexedIndirect(const Buffer& buffer,
                                        VkDeviceSize offset,
                                        uint32_t drawCount,
                                        uint32_t stride) const
{
    vkCmdDrawIndexedIndirect(m_CmdBuffer, buffer.GetVkHandle(), offset, drawCount, stride);
}

} // namespace Grace
