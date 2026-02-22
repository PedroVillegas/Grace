#include <Grace/GpuResourceHandles.hpp>
#include <Private/Grace/InternalContainers.hpp>
#include <Private/Grace/Assert.hpp>

namespace Grace
{

std::unique_ptr<AbandonedResources> gAbandonedResources = nullptr;
std::unique_ptr<ResourceHandleRefCounters> gResHandleRefCounters = nullptr;

template <typename ResourceType>
RefCountedHandle<ResourceType>::RefCountedHandle(uint32_t UUID, uint32_t Validator)
    : mHandle(UUID), mValidator31Alive1(Validator)
{
    if (IsAlive())
    {
        InstantiateRefCounter();
    }
}

template <typename ResourceType>
RefCountedHandle<ResourceType>::~RefCountedHandle()
{
    const uint32_t refcount = AdjustRefCounter(-1);
    if (refcount == 0)
    {
        AbandonHandle();
    }
}

template <typename ResourceType>
RefCountedHandle<ResourceType>& RefCountedHandle<ResourceType>::operator=(const RefCountedHandle& rhs)
{
    mHandle = rhs.mHandle;
    mValidator31Alive1 = rhs.mValidator31Alive1;
    [[maybe_unused]] const uint32_t refcountNew = AdjustRefCounter(1);
    return *this;
}

template <typename ResourceType>
RefCountedHandle<ResourceType>::RefCountedHandle(const RefCountedHandle& rhs)
    : mHandle(rhs.mHandle), mValidator31Alive1(rhs.mValidator31Alive1)
{
    [[maybe_unused]] const uint32_t refcount = AdjustRefCounter(1);
}

template <typename ResourceType>
RefCountedHandle<ResourceType>& RefCountedHandle<ResourceType>::operator=(RefCountedHandle&& rhs) noexcept
{
    mHandle = rhs.mHandle;
    mValidator31Alive1 = rhs.mValidator31Alive1;
    [[maybe_unused]] const uint32_t refcountNew = AdjustRefCounter(1);
    return *this;
}

template <typename ResourceType>
RefCountedHandle<ResourceType>::RefCountedHandle(RefCountedHandle&& rhs) noexcept
    : mHandle(rhs.mHandle), mValidator31Alive1(rhs.mValidator31Alive1)
{
    [[maybe_unused]] const uint32_t refcount = AdjustRefCounter(1);
}

template <typename ResourceType>
void RefCountedHandle<ResourceType>::AbandonHandle() const
{
    gAbandonedResources->any = true;
    uint32_t dead = mValidator31Alive1 | 0x1;

    if constexpr (std::is_same_v<ResourceType, Buffer>)
    {
        gAbandonedResources->buffers.emplace_back(mHandle, dead);
    }
    else if constexpr (std::is_same_v<ResourceType, Image>)
    {
        gAbandonedResources->images.emplace_back(mHandle, dead);
    }
    else if constexpr (std::is_same_v<ResourceType, Sampler>)
    {
        gAbandonedResources->samplers.emplace_back(mHandle, dead);
    }
    else if constexpr (std::is_same_v<ResourceType, Pipeline>)
    {
        gAbandonedResources->pipelines.emplace_back(mHandle, dead);
    }
    else if constexpr (std::is_same_v<ResourceType, PipelineLayout>)
    {
        gAbandonedResources->pipelineLayouts.emplace_back(mHandle, dead);
    }
    else if constexpr (std::is_same_v<ResourceType, Fence>)
    {
        gAbandonedResources->fences.emplace_back(mHandle, dead);
    }
    else if constexpr (std::is_same_v<ResourceType, Semaphore<SemaphoreType::Binary>>)
    {
        gAbandonedResources->binarySemaphores.emplace_back(mHandle, dead);
    }
    else if constexpr (std::is_same_v<ResourceType, Semaphore<SemaphoreType::Timeline>>)
    {
        gAbandonedResources->timelineSemaphores.emplace_back(mHandle, dead);
    }
}

template <typename ResourceType>
void RefCountedHandle<ResourceType>::InstantiateRefCounter() const
{
    if constexpr (std::is_same_v<ResourceType, Buffer>)
    {
        gResHandleRefCounters->buffers.emplace_back(1);
    }
    else if constexpr (std::is_same_v<ResourceType, Image>)
    {
        gResHandleRefCounters->images.emplace_back(1);
    }
    else if constexpr (std::is_same_v<ResourceType, Sampler>)
    {
        gResHandleRefCounters->samplers.emplace_back(1);
    }
    else if constexpr (std::is_same_v<ResourceType, Pipeline>)
    {
        gResHandleRefCounters->pipelines.emplace_back(1);
    }
    else if constexpr (std::is_same_v<ResourceType, PipelineLayout>)
    {
        gResHandleRefCounters->pipelineLayouts.emplace_back(1);
    }
    else if constexpr (std::is_same_v<ResourceType, Fence>)
    {
        gResHandleRefCounters->fences.emplace_back(1);
    }
    else if constexpr (std::is_same_v<ResourceType, Semaphore<SemaphoreType::Binary>>)
    {
        gResHandleRefCounters->binarySemaphores.emplace_back(1);
    }
    else if constexpr (std::is_same_v<ResourceType, Semaphore<SemaphoreType::Timeline>>)
    {
        gResHandleRefCounters->timelineSemaphores.emplace_back(1);
    }
}

template <typename ResourceType>
uint32_t RefCountedHandle<ResourceType>::AdjustRefCounter(int count) const
{
    if (!HasValidHandle() || !IsAlive())
    {
        return ~0U;
    }

    uint32_t* current = nullptr;
    if constexpr (std::is_same_v<ResourceType, Buffer>)
    {
        current = &gResHandleRefCounters->buffers.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<ResourceType, Image>)
    {
        current = &gResHandleRefCounters->images.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<ResourceType, Sampler>)
    {
        current = &gResHandleRefCounters->samplers.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<ResourceType, Pipeline>)
    {
        current = &gResHandleRefCounters->pipelines.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<ResourceType, PipelineLayout>)
    {
        current = &gResHandleRefCounters->pipelineLayouts.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<ResourceType, Fence>)
    {
        current = &gResHandleRefCounters->fences.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<ResourceType, Semaphore<SemaphoreType::Binary>>)
    {
        current = &gResHandleRefCounters->binarySemaphores.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<ResourceType, Semaphore<SemaphoreType::Timeline>>)
    {
        current = &gResHandleRefCounters->timelineSemaphores.at(mHandle);
        *current += count;
    }
    return *current;
}

template class GRACE_API RefCountedHandle<Buffer>;
template class GRACE_API RefCountedHandle<Image>;
template class GRACE_API RefCountedHandle<Sampler>;
template class GRACE_API RefCountedHandle<Pipeline>;
template class GRACE_API RefCountedHandle<PipelineLayout>;
template class GRACE_API RefCountedHandle<Fence>;
template class GRACE_API RefCountedHandle<Semaphore<SemaphoreType::Binary>>;
template class GRACE_API RefCountedHandle<Semaphore<SemaphoreType::Timeline>>;

} // namespace Grace
