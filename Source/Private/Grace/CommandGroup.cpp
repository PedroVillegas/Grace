#include <Grace/CommandGroup.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/Context.hpp>
#include <Grace/HelperFunctions.hpp>
#include <Private/Grace/SyncGroup.hpp>
#include <Grace/Assert.hpp>

namespace Grace
{

static BarrierBuilder mBarrierBuilder = {};

CommandGroupAllocator::~CommandGroupAllocator() = default;

CommandGroupAllocator::CommandGroupAllocator() = default;

CommandGroupAllocator::CommandGroupAllocator(Device* device) : mDevicePtr(device) {}

CommandGroupAllocator::CommandGroupAllocator(CommandGroupAllocator&& other) noexcept
    : mDevicePtr(other.mDevicePtr), mAllCommandPoolsAllocated(std::move(other.mAllCommandPoolsAllocated)),
      mFreeCommandPools(std::move(other.mFreeCommandPools))
{}

CommandGroupAllocator& CommandGroupAllocator::operator=(CommandGroupAllocator&& other) noexcept
{
    mDevicePtr = other.mDevicePtr;
    mAllCommandPoolsAllocated = std::move(other.mAllCommandPoolsAllocated);
    mFreeCommandPools = std::move(other.mFreeCommandPools);

    return *this;
}

CommandPool* CommandGroupAllocator::GetOrAllocateCommandPool(QueueFamily queueFamily, const char* name)
{
    GRACE_ASSERT(static_cast<uint32_t>(queueFamily) <= static_cast<uint32_t>(QueueFamily::Graphics));

    // If no pools are free, allocate new pool
    std::deque<CommandPool>& allocatedPoolsOfQueueFamily =
        mAllCommandPoolsAllocated[static_cast<uint32_t>(queueFamily)];
    if (mFreeCommandPools.empty())
    {
        const VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = mDevicePtr->GetQueueFamilyIndex(queueFamily),
        };

        VkCommandPool allocatedCmdPool = nullptr;
        DebugReporter::Check(vkCreateCommandPool(mDevicePtr->VkHandle(), &poolInfo, nullptr, &allocatedCmdPool));

        allocatedPoolsOfQueueFamily.emplace_back(mDevicePtr, allocatedCmdPool, queueFamily);
        mFreeCommandPools.push(&allocatedPoolsOfQueueFamily.back());
    }

    GRACE_ASSERT(!mFreeCommandPools.empty());
    CommandPool* ret = mFreeCommandPools.front();
    mFreeCommandPools.pop();

    AssignDebugName<VkCommandPool>(mDevicePtr->VkHandle(), ret->GetVkCommandPool(), name);

    return ret;
}

void CommandGroupAllocator::ReturnCommandPool(CommandPool* commandPool)
{
    mFreeCommandPools.push(commandPool);
}

void CommandGroupAllocator::FreeCommandPool() {}

void CommandGroupAllocator::FreeCommandBuffer() {}

CommandPool::~CommandPool()
{
    if (mCommandPool != nullptr)
    {
        vkDestroyCommandPool(mDevice->VkHandle(), mCommandPool, nullptr);
    }
}

CommandPool::CommandPool(Device* pDevice, VkCommandPool commandPool, QueueFamily queueFamily)
    : mDevice(pDevice), mCommandPool(commandPool), mQueueFamily(queueFamily)
{}

CommandPool::CommandPool(CommandPool&& other) noexcept
    : mDevice(other.mDevice), mCommandPool(other.mCommandPool), mQueueFamily(other.mQueueFamily),
      mCommandBuffers(std::move(other.mCommandBuffers)), mCommandBuffersInUse(other.mCommandBuffersInUse)
{
    other.mCommandPool = nullptr;
}

CommandPool& CommandPool::operator=(CommandPool&& other) noexcept
{
    mDevice = other.mDevice;
    mCommandPool = other.mCommandPool;
    mQueueFamily = other.mQueueFamily;
    mCommandBuffers = std::move(other.mCommandBuffers);
    mCommandBuffersInUse = other.mCommandBuffersInUse;
    other.mCommandPool = nullptr;

    return *this;
}

void CommandPool::Reset()
{
    vkResetCommandPool(mDevice->VkHandle(), mCommandPool, 0);
}

CommandBuffer CommandPool::GetOrAllocateCommandBuffer()
{
    if (mCommandBuffers.size() == mCommandBuffersInUse)
    {
        const VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = mCommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer allocated = nullptr;
        DebugReporter::Check(vkAllocateCommandBuffers(mDevice->VkHandle(), &allocInfo, &allocated));

        mCommandBuffers.push_back(allocated);
    }

    return { mDevice, mCommandBuffers[mCommandBuffersInUse++], mQueueFamily, mDevice->GetQueryManagerPtr() };
}

QueueFamily CommandPool::GetQueueFamily() const
{
    return mQueueFamily;
}

VkCommandPool CommandPool::GetVkCommandPool() const
{
    return mCommandPool;
}

CommandBuffer::CommandBuffer(Device* pDevice,
                             VkCommandBuffer commandBuffer,
                             QueueFamily queueFamily,
                             QueryManager* pQueryMgr)
    : mDevicePtr(pDevice), mCmdBuffer(commandBuffer), mQueueFamily(queueFamily), mQueryMgrPtr(pQueryMgr)
{}

bool CommandBuffer::IsNull() const
{
    return mCmdBuffer == nullptr;
}

const VkCommandBuffer& CommandBuffer::GetVkCommandBuffer() const
{
    return mCmdBuffer;
}

void CommandBuffer::Reset(VkCommandBufferResetFlags resetFlags) const
{
    DebugReporter::Check(vkResetCommandBuffer(mCmdBuffer, resetFlags));
}

void CommandBuffer::BeginRecording(VkCommandBufferUsageFlags usageFlags,
                                   const VkCommandBufferInheritanceInfo* pInheritanceInfo) const
{
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = usageFlags,
        .pInheritanceInfo = pInheritanceInfo,
    };

    DebugReporter::Check(vkBeginCommandBuffer(mCmdBuffer, &beginInfo));
}

void CommandBuffer::EndRecording() const
{
    DebugReporter::Check(vkEndCommandBuffer(mCmdBuffer));
}

void CommandBuffer::BindPipeline(GraphicsPipelineHandle pipeline) const
{
    const GraphicsPipeline& p = mDevicePtr->Get<GraphicsPipeline>(pipeline);
    GRACE_ASSERT(!p.Exists());

    vkCmdBindPipeline(mCmdBuffer, p.BindPoint(), p.VkHandle());
}

void CommandBuffer::BindPipeline(ComputePipelineHandle pipeline) const
{
    const ComputePipeline& p = mDevicePtr->Get<ComputePipeline>(pipeline);
    GRACE_ASSERT(!p.Exists());

    vkCmdBindPipeline(mCmdBuffer, p.BindPoint(), p.VkHandle());
}

void CommandBuffer::BindDescriptorSets(PipelineBindPoint bindpoint,
                                       PipelineLayoutHandle layout,
                                       const std::initializer_list<VkDescriptorSet>&& descriptorSets) const
{
    const PipelineLayout& pl = mDevicePtr->Get<PipelineLayout>(layout);
    GRACE_ASSERT(!pl.Exists());
    vkCmdBindDescriptorSets(mCmdBuffer,
                            static_cast<VkPipelineBindPoint>(bindpoint),
                            pl.VkHandle(),
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            descriptorSets.begin(),
                            0,
                            nullptr);
}

void CommandBuffer::PushConstants(PipelineLayoutHandle layout, uint32_t size, const void* data) const
{
    const PipelineLayout& pl = mDevicePtr->Get<PipelineLayout>(layout);
    GRACE_ASSERT(!pl.Exists());
    GRACE_ASSERT(size <= 128);
    vkCmdPushConstants(mCmdBuffer, pl.VkHandle(), VK_SHADER_STAGE_ALL, 0, size, data);
}

void CommandBuffer::BeginDebugLabel(const char* label, const Float4& colour) const
{
    const VkDebugUtilsLabelEXT labelInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = { colour.x, colour.y, colour.z, colour.w },
    };

    vkCmdBeginDebugUtilsLabelEXT_Meta(mCmdBuffer, &labelInfo);
}

void CommandBuffer::InsertDebugLabel(const char* label, const Float4& colour) const
{
    const VkDebugUtilsLabelEXT labelInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = { colour.x, colour.y, colour.z, colour.w },
    };

    vkCmdInsertDebugUtilsLabelEXT_Meta(mCmdBuffer, &labelInfo);
}

void CommandBuffer::EndDebugLabel() const
{
    vkCmdEndDebugUtilsLabelEXT_Meta(mCmdBuffer);
}

void CommandBuffer::AddBufferBarrier(BufferHandle buffer,
                                     std::vector<AccessType>&& accessesBefore,
                                     std::vector<AccessType>&& accessesAfter)
{
    const Buffer& buf = mDevicePtr->Get<Buffer>(buffer);
    mBarrierBuilder.AddBufferBarrier(buf, std::move(accessesBefore), std::move(accessesAfter));
}

void CommandBuffer::AddImageBarrier(ImageHandle image,
                                    std::vector<AccessType>&& accessesBefore,
                                    std::vector<AccessType>&& accessesAfter)
{
    const Image& img = mDevicePtr->Get<Image>(image);
    mBarrierBuilder.AddImageBarrier(img, std::move(accessesBefore), std::move(accessesAfter));
}

void CommandBuffer::AddMemoryBarrier(std::vector<AccessType>&& accessesBefore, std::vector<AccessType>&& accessesAfter)
{
    mBarrierBuilder.AddMemoryBarrier(std::move(accessesBefore), std::move(accessesAfter));
}

void CommandBuffer::PipelineBarrier()
{
    mBarrierBuilder.PipelineBarrier(mCmdBuffer);
}

void CommandBuffer::BeginDynamicRendering(const DynamicRenderingDesc&& desc) const
{
    GRACE_ASSERT(desc.colorAttachments.size() > 0);
    GRACE_ASSERT(desc.renderArea.extent.width > 0 && desc.renderArea.extent.height > 0);

    const VkRenderingInfo renderInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = desc.flags,
        .renderArea = VkRect2D(desc.renderArea.offset, desc.renderArea.extent),
        .layerCount = desc.layerCount,
        .viewMask = desc.viewMask,
        .colorAttachmentCount = static_cast<uint32_t>(desc.colorAttachments.size()),
        .pColorAttachments = desc.colorAttachments.begin(),
        .pDepthAttachment = desc.depthAttachments.begin(),
        .pStencilAttachment = desc.stencilAttachments.begin(),
    };

    vkCmdBeginRendering(mCmdBuffer, &renderInfo);
}

void CommandBuffer::EndDynamicRendering() const
{
    vkCmdEndRendering(mCmdBuffer);
}

void CommandBuffer::SetViewport(const std::initializer_list<VkViewport>&& viewports) const
{
    vkCmdSetViewport(mCmdBuffer, 0, static_cast<uint32_t>(viewports.size()), viewports.begin());
}

void CommandBuffer::SetScissor(const std::initializer_list<VkRect2D>&& scissors) const
{
    vkCmdSetScissor(mCmdBuffer, 0, static_cast<uint32_t>(scissors.size()), scissors.begin());
}

void CommandBuffer::BindIndexBuffer(BufferHandle buffer, VkDeviceSize offset, IndexType indexType) const
{
    const Buffer& buf = mDevicePtr->Get<Buffer>(buffer);
    GRACE_ASSERT(!buf.Exists());
    vkCmdBindIndexBuffer(mCmdBuffer, buf.VkHandle(), offset, static_cast<VkIndexType>(indexType));
}

void CommandBuffer::Draw(uint32_t vertexCount,
                         uint32_t instanceCount,
                         uint32_t firstVertex,
                         uint32_t firstInstance) const
{
    vkCmdDraw(mCmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::DrawIndexed(uint32_t indexCount,
                                uint32_t instanceCount,
                                uint32_t firstIndex,
                                int32_t vertexOffset,
                                uint32_t firstInstance) const
{
    vkCmdDrawIndexed(mCmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::DrawIndexedIndirect(BufferHandle buffer,
                                        VkDeviceSize offset,
                                        uint32_t drawCount,
                                        uint32_t stride) const
{
    const Buffer& buf = mDevicePtr->Get<Buffer>(buffer);
    GRACE_ASSERT(!buf.Exists());
    vkCmdDrawIndexedIndirect(mCmdBuffer, buf.VkHandle(), offset, drawCount, stride);
}

void CommandBuffer::Dispatch(uint32_t x, uint32_t y, uint32_t z) const
{
    vkCmdDispatch(mCmdBuffer, x, y, z);
}

void CommandBuffer::DispatchIndirect(BufferHandle buffer, uint64_t offset) const
{
    const Buffer& buf = mDevicePtr->Get<Buffer>(buffer);
    GRACE_ASSERT(!buf.Exists());
    vkCmdDispatchIndirect(mCmdBuffer, buf.VkHandle(), offset);
}

void CommandBuffer::BlitImage(const VkBlitImageInfo2& blitInfo) const
{
    vkCmdBlitImage2(mCmdBuffer, &blitInfo);
}

void CommandBuffer::ClearColorImage(ImageHandle image, const ClearColourValue& color) const
{
    const Image& img = mDevicePtr->Get<Image>(image);
    GRACE_ASSERT(!img.Exists());

    const VkImageSubresourceRange subresourceRange = {
        .aspectMask = static_cast<VkImageAspectFlags>(img.InferAspect()),
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    vkCmdClearColorImage(
        mCmdBuffer, img.VkHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color.clear, 1, &subresourceRange);
}

void CommandBuffer::ClearColorImageRanges(ImageHandle image,
                                          const ClearColourValue& color,
                                          const std::initializer_list<VkImageSubresourceRange>&& ranges) const
{
    const Image& img = mDevicePtr->Get<Image>(image);
    GRACE_ASSERT(!img.Exists());
    vkCmdClearColorImage(mCmdBuffer,
                         img.VkHandle(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         &color.clear,
                         static_cast<uint32_t>(ranges.size()),
                         ranges.begin());
}

void CommandBuffer::CopyBufferToImage(BufferHandle buffer, ImageHandle image, VkImageLayout dstLayout) const
{
    const Buffer& buf = mDevicePtr->Get<Buffer>(buffer);
    const Image& img = mDevicePtr->Get<Image>(image);
    GRACE_ASSERT(!buf.Exists());
    GRACE_ASSERT(!img.Exists());

    const UInt3& imgext = img.GetExtent3D();
    const VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = static_cast<VkImageAspectFlags>(img.InferAspect()),
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        },
        .imageOffset = VkOffset3D(0, 0, 0),
        .imageExtent = VkExtent3D(imgext.x, imgext.y, imgext.z),
    };

    vkCmdCopyBufferToImage(mCmdBuffer, buf.VkHandle(), img.VkHandle(), dstLayout, 1, &region);
}

void CommandBuffer::CopyBufferToImageRegions(BufferHandle buffer,
                                             ImageHandle image,
                                             VkImageLayout dstLayout,
                                             const std::initializer_list<VkBufferImageCopy>&& regions) const
{
    const Buffer& buf = mDevicePtr->Get<Buffer>(buffer);
    const Image& img = mDevicePtr->Get<Image>(image);
    GRACE_ASSERT(!buf.Exists());
    GRACE_ASSERT(!img.Exists());
    vkCmdCopyBufferToImage(mCmdBuffer,
                           buf.VkHandle(),
                           img.VkHandle(),
                           dstLayout,
                           static_cast<uint32_t>(regions.size()),
                           regions.begin());
}

void CommandBuffer::CopyBuffer(BufferHandle srcBuffer, BufferHandle dstBuffer) const
{
    const Buffer& srcbuf = mDevicePtr->Get<Buffer>(srcBuffer);
    const Buffer& dstbuf = mDevicePtr->Get<Buffer>(dstBuffer);

    GRACE_ASSERT(!srcbuf.Exists());
    GRACE_ASSERT(!dstbuf.Exists());
    GRACE_ASSERT(srcbuf.GetAllocationInfo().allocationInfo.size == dstbuf.GetAllocationInfo().allocationInfo.size);

    const VkBufferCopy region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = srcbuf.GetAllocationInfo().allocationInfo.size,
    };

    vkCmdCopyBuffer(mCmdBuffer, srcbuf.VkHandle(), dstbuf.VkHandle(), 1, &region);
}

void CommandBuffer::CopyBufferRanges(BufferHandle srcBuffer,
                                     BufferHandle dstBuffer,
                                     const std::initializer_list<VkBufferCopy>&& regions) const
{
    const Buffer& srcbuf = mDevicePtr->Get<Buffer>(srcBuffer);
    const Buffer& dstbuf = mDevicePtr->Get<Buffer>(dstBuffer);
    GRACE_ASSERT(!srcbuf.Exists());
    GRACE_ASSERT(!dstbuf.Exists());
    vkCmdCopyBuffer(
        mCmdBuffer, srcbuf.VkHandle(), dstbuf.VkHandle(), static_cast<uint32_t>(regions.size()), regions.begin());
}

void CommandBuffer::FillBuffer(BufferHandle buffer, uint32_t data, VkDeviceSize offset, VkDeviceSize size) const
{
    const Buffer& buf = mDevicePtr->Get<Buffer>(buffer);
    vkCmdFillBuffer(mCmdBuffer, buf.VkHandle(), offset, size, data);
}

void CommandBuffer::WriteTimestamp(const char* name, PipelineStage stage, uint32_t frameIndex) const
{
    const TimestampQueryGroup& qg = mQueryMgrPtr->GetQueryGroup<QueryType::Timestamp>();
    const uint32_t query = mQueryMgrPtr->AddQuery<QueryType::Timestamp>(name);

    const uint32_t offset = frameIndex * (qg.GetRange() - 1);

    vkCmdWriteTimestamp2(mCmdBuffer, static_cast<VkPipelineStageFlags2>(stage), qg.GetVkQueryPool(), offset + query);
}

} // namespace Grace
