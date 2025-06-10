#include "CommandGroup.hpp"

#include <Grace/DebugReporter.hpp>
#include <Grace/Context.hpp>
#include <Grace/HelperFunctions.hpp>

#include <cassert>

namespace Grace
{

void CommandGroupAllocator::Initialise(Device* device)
{
    m_Device = device;
}

void CommandGroupAllocator::FreeAllRemaining()
{
    assert(m_Device->GetVkDevice() != nullptr);

    for (auto& queueFamily : m_AllCommandPoolsAllocated)
    {
        for (auto& commandPool : queueFamily)
        {
            commandPool.DestroyIfNotNull(m_Device->GetVkDevice());
        }
    }
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
        DebugReporter::Check(vkCreateCommandPool(m_Device->GetVkDevice(), &poolInfo, nullptr, &allocatedCmdPool));

        allocatedPoolsOfQueueFamily.emplace_back(allocatedCmdPool, queueFamily);
        m_FreeCommandPools.push(&allocatedPoolsOfQueueFamily.back());
    }

    assert(m_FreeCommandPools.size() > 0);
    CommandPool* ret = m_FreeCommandPools.front();
    m_FreeCommandPools.pop();

    AssignDebugName<VkCommandPool>(m_Device->GetVkDevice(), ret->GetVkCommandPool(), VK_OBJECT_TYPE_COMMAND_POOL, name);

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

CommandPool::CommandPool(VkCommandPool commandPool, QueueFamily queueFamily)
{
    m_CommandPool = commandPool;
    m_QueueFamily = queueFamily;
}

void CommandPool::DestroyIfNotNull(VkDevice device)
{
    if (m_CommandPool != nullptr)
    {
        vkDestroyCommandPool(device, m_CommandPool, nullptr);
    }
}

void CommandPool::Reset(Device* device)
{
    vkResetCommandPool(device->GetVkDevice(), m_CommandPool, 0);
}

CommandBuffer CommandPool::GetOrAllocateCommandBuffer(Device* device)
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
        DebugReporter::Check(vkAllocateCommandBuffers(device->GetVkDevice(), &allocInfo, &allocated));

        m_CommandBuffers.push_back(allocated);
    }

    return CommandBuffer(m_CommandBuffers[m_CommandBuffersInUse++]);
}

QueueFamily CommandPool::GetQueueFamily() const
{
    return m_QueueFamily;
}

VkCommandPool CommandPool::GetVkCommandPool() const
{
    return m_CommandPool;
}

CommandBuffer::CommandBuffer(VkCommandBuffer cmdBuffer)
{
    Set(cmdBuffer);
}

void CommandBuffer::Set(VkCommandBuffer cmdBuffer)
{
    m_CmdBuffer = cmdBuffer;
}

VkCommandBuffer CommandBuffer::GetVkCommandBuffer() const
{
    return m_CmdBuffer;
}

bool CommandBuffer::IsNull() const
{
    return m_CmdBuffer == nullptr;
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

void CommandBuffer::CmdBindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,
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

void CommandBuffer::CmdBeginDynamicRendering(const DynamicRenderingDesc& desc) const
{
    assert(desc.colorAttachments.size() > 0);
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

void CommandBuffer::CmdEndDynamicRendering() const
{
    vkCmdEndRendering(m_CmdBuffer);
}

void CommandBuffer::CmdSetViewport(const std::vector<VkViewport>& viewports, uint32_t firstViewport) const
{
    assert(viewports.size() > 0);
    vkCmdSetViewport(m_CmdBuffer, firstViewport, static_cast<uint32_t>(viewports.size()), viewports.data());
}

void CommandBuffer::CmdSetScissor(const std::vector<VkRect2D>& scissors, uint32_t firstScissor) const
{
    assert(scissors.size() > 0);
    vkCmdSetScissor(m_CmdBuffer, firstScissor, static_cast<uint32_t>(scissors.size()), scissors.data());
}

void CommandBuffer::CmdPushConstants(VkPipelineLayout pipelineLayout, uint32_t size, const void* data) const
{
    assert(pipelineLayout != nullptr);
    assert(size <= 128);
    vkCmdPushConstants(m_CmdBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, size, data);
}

void CommandBuffer::CmdAddImageLayoutTransition(const Image& image,
                                                ThsvsAccessType prevAccess,
                                                ThsvsAccessType nextAccess,
                                                VkImageAspectFlags aspectMask)
{
    m_BarrierBuilder.AddImageLayoutTransition(image, prevAccess, nextAccess, aspectMask);
}

void CommandBuffer::CmdAddMemoryBarrier(ThsvsAccessType prevAccess, ThsvsAccessType nextAccess)
{
    m_BarrierBuilder.AddMemoryBarrier(prevAccess, nextAccess);
}

void CommandBuffer::CmdExecuteBarriers()
{
    m_BarrierBuilder.ExecutePipelineBarrier(m_CmdBuffer);
}

void CommandBuffer::CmdBindGraphicsPipeline(const Pipeline& pipeline) const
{
    assert(pipeline.pipeline != nullptr);

    vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
}

void CommandBuffer::CmdBindComputePipeline(const Pipeline& pipeline) const
{
    assert(pipeline.pipeline != nullptr);

    vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
}

void CommandBuffer::CmdDispatch(uint32_t x, uint32_t y, uint32_t z) const
{
    vkCmdDispatch(m_CmdBuffer, x, y, z);
}

void CommandBuffer::CmdDispatchIndirect(const Buffer& buffer, uint64_t offset) const
{
    vkCmdDispatchIndirect(m_CmdBuffer, buffer.GetBuffer(), offset);
}

void CommandBuffer::CmdBlitImage(const VkBlitImageInfo2& blitInfo) const
{
    vkCmdBlitImage2(m_CmdBuffer, &blitInfo);
}

void CommandBuffer::CmdCopyBufferToImage(const Buffer& buffer,
                                         const Image& image,
                                         VkImageLayout dstLayout,
                                         const std::vector<VkBufferImageCopy>& regions) const
{
    assert(!buffer.IsNull());
    assert(!image.IsNull());
    assert(regions.size() > 0);
    vkCmdCopyBufferToImage(m_CmdBuffer,
                           buffer.GetBuffer(),
                           image.GetImage(),
                           dstLayout,
                           static_cast<uint32_t>(regions.size()),
                           regions.data());
}

void CommandBuffer::CmdCopyBuffer(const Buffer& srcBuffer,
                                  const Buffer& dstBuffer,
                                  const std::vector<VkBufferCopy>& regions) const
{
    assert(!srcBuffer.IsNull());
    assert(!dstBuffer.IsNull());
    assert(regions.size() > 0);
    vkCmdCopyBuffer(m_CmdBuffer,
                    srcBuffer.GetBuffer(),
                    dstBuffer.GetBuffer(),
                    static_cast<uint32_t>(regions.size()),
                    regions.data());
}

void CommandBuffer::CmdFillBuffer(const Buffer& buffer, uint32_t data, VkDeviceSize offset, VkDeviceSize size) const
{
    vkCmdFillBuffer(m_CmdBuffer, buffer.GetBuffer(), offset, size, data);
}

void CommandBuffer::CmdClearColorImage(const Image& image,
                                       const VkClearColorValue* pColor,
                                       const std::vector<VkImageSubresourceRange>& ranges) const
{
    vkCmdClearColorImage(m_CmdBuffer,
                         image.GetImage(),
                         VK_IMAGE_LAYOUT_GENERAL,
                         pColor,
                         static_cast<uint32_t>(ranges.size()),
                         ranges.data());
}

void CommandBuffer::CmdBindIndexBuffer(const Buffer& buffer, VkDeviceSize offset, VkIndexType indexType) const
{
    vkCmdBindIndexBuffer(m_CmdBuffer, buffer.GetBuffer(), offset, indexType);
}

void CommandBuffer::CmdDraw(uint32_t vertexCount,
                            uint32_t instanceCount,
                            uint32_t firstVertex,
                            uint32_t firstInstance) const
{
    vkCmdDraw(m_CmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::CmdDrawIndexed(uint32_t indexCount,
                                   uint32_t instanceCount,
                                   uint32_t firstIndex,
                                   int32_t vertexOffset,
                                   uint32_t firstInstance) const
{
    vkCmdDrawIndexed(m_CmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::CmdDrawIndexedIndirect(const Buffer& buffer,
                                           VkDeviceSize offset,
                                           uint32_t drawCount,
                                           uint32_t stride) const
{
    vkCmdDrawIndexedIndirect(m_CmdBuffer, buffer.GetBuffer(), offset, drawCount, stride);
}

} // namespace Grace
