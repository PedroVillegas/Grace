#include <Grace/GpuHandle.hpp>
#include <Grace/Detail/InternalContainers.hpp>
#include <Grace/Detail/Logging.hpp>

namespace Grace
{

std::unique_ptr<AbandonedResources> gAbandonedResources = nullptr;
std::unique_ptr<ResourceHandleRefCounters> gResHandleRefCounters = nullptr;

template <typename T>
GpuHandle<T>::GpuHandle(uint32_t UUID, uint32_t Validator, bool RefCounted) : mHandle(UUID), mGeneration(Validator)
{
    if (RefCounted)
    {
        mGeneration |= (1u << 31u);
    }

    // Must always instantiate ref counter so the handle can be used directly as index
    InstantiateRefCounter();
}

template <typename T>
GpuHandle<T>::~GpuHandle()
{
    if (!RefCounted())
    {
        return;
    }

    const uint32_t refcount = AdjustRefCounter(-1);
    if (refcount == 0)
    {
        AbandonHandle();
    }
}

template <typename T>
GpuHandle<T>& GpuHandle<T>::operator=(const GpuHandle& rhs)
{
    mHandle = rhs.mHandle;
    mGeneration = rhs.mGeneration;

    if (RefCounted())
    {
        [[maybe_unused]] const uint32_t refcountNew = AdjustRefCounter(1);
    }
    return *this;
}

template <typename T>
GpuHandle<T>::GpuHandle(const GpuHandle& rhs) : mHandle(rhs.mHandle), mGeneration(rhs.mGeneration)
{
    if (RefCounted())
    {
        [[maybe_unused]] const uint32_t refcount = AdjustRefCounter(1);
    }
}

template <typename T>
GpuHandle<T>& GpuHandle<T>::operator=(GpuHandle&& rhs) noexcept
{
    mHandle = rhs.mHandle;
    mGeneration = rhs.mGeneration;

    if (RefCounted())
    {
        [[maybe_unused]] const uint32_t refcountNew = AdjustRefCounter(1);
    }
    return *this;
}

template <typename T>
GpuHandle<T>::GpuHandle(GpuHandle&& rhs) noexcept : mHandle(rhs.mHandle), mGeneration(rhs.mGeneration)
{
    if (RefCounted())
    {
        [[maybe_unused]] const uint32_t refcount = AdjustRefCounter(1);
    }
}

template <typename T>
void GpuHandle<T>::AbandonHandle() const
{
    gAbandonedResources->any = true;
    uint32_t dead = mGeneration | (1u << 30u);

    if constexpr (std::is_same_v<T, Buffer>)
    {
        gAbandonedResources->buffers.emplace_back(mHandle, dead, true);
    }
    else if constexpr (std::is_same_v<T, Image>)
    {
        gAbandonedResources->images.emplace_back(mHandle, dead, true);
    }
    else if constexpr (std::is_same_v<T, Sampler>)
    {
        gAbandonedResources->samplers.emplace_back(mHandle, dead, true);
    }
    else if constexpr (std::is_same_v<T, Pipeline<PipelineType::Graphics>>)
    {
        gAbandonedResources->graphicsPipelines.emplace_back(mHandle, dead, true);
    }
    else if constexpr (std::is_same_v<T, Pipeline<PipelineType::Compute>>)
    {
        gAbandonedResources->computePipelines.emplace_back(mHandle, dead, true);
    }
    else if constexpr (std::is_same_v<T, PipelineLayout>)
    {
        gAbandonedResources->pipelineLayouts.emplace_back(mHandle, dead, true);
    }
    else if constexpr (std::is_same_v<T, Fence>)
    {
        gAbandonedResources->fences.emplace_back(mHandle, dead, true);
    }
    else if constexpr (std::is_same_v<T, Semaphore<SemaphoreType::Binary>>)
    {
        gAbandonedResources->binarySemaphores.emplace_back(mHandle, dead, true);
    }
    else if constexpr (std::is_same_v<T, Semaphore<SemaphoreType::Timeline>>)
    {
        gAbandonedResources->timelineSemaphores.emplace_back(mHandle, dead, true);
    }
}

template <typename T>
void GpuHandle<T>::InstantiateRefCounter() const
{
    if constexpr (std::is_same_v<T, Buffer>)
    {
        gResHandleRefCounters->buffers.emplace_back(1);
    }
    else if constexpr (std::is_same_v<T, Image>)
    {
        gResHandleRefCounters->images.emplace_back(1);
    }
    else if constexpr (std::is_same_v<T, Sampler>)
    {
        gResHandleRefCounters->samplers.emplace_back(1);
    }
    else if constexpr (std::is_same_v<T, Pipeline<PipelineType::Graphics>>)
    {
        gResHandleRefCounters->graphicsPipelines.emplace_back(1);
    }
    else if constexpr (std::is_same_v<T, Pipeline<PipelineType::Compute>>)
    {
        gResHandleRefCounters->computePipelines.emplace_back(1);
    }
    else if constexpr (std::is_same_v<T, PipelineLayout>)
    {
        gResHandleRefCounters->pipelineLayouts.emplace_back(1);
    }
    else if constexpr (std::is_same_v<T, Fence>)
    {
        gResHandleRefCounters->fences.emplace_back(1);
    }
    else if constexpr (std::is_same_v<T, Semaphore<SemaphoreType::Binary>>)
    {
        gResHandleRefCounters->binarySemaphores.emplace_back(1);
    }
    else if constexpr (std::is_same_v<T, Semaphore<SemaphoreType::Timeline>>)
    {
        gResHandleRefCounters->timelineSemaphores.emplace_back(1);
    }
}

template <typename T>
uint32_t GpuHandle<T>::AdjustRefCounter(int count) const
{
    if (!Exists() || !IsAlive())
    {
        return ~0U;
    }

    uint32_t* current = nullptr;
    if constexpr (std::is_same_v<T, Buffer>)
    {
        current = &gResHandleRefCounters->buffers.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<T, Image>)
    {
        current = &gResHandleRefCounters->images.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<T, Sampler>)
    {
        current = &gResHandleRefCounters->samplers.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<T, Pipeline<PipelineType::Graphics>>)
    {
        current = &gResHandleRefCounters->graphicsPipelines.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<T, Pipeline<PipelineType::Compute>>)
    {
        current = &gResHandleRefCounters->computePipelines.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<T, PipelineLayout>)
    {
        current = &gResHandleRefCounters->pipelineLayouts.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<T, Fence>)
    {
        current = &gResHandleRefCounters->fences.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<T, Semaphore<SemaphoreType::Binary>>)
    {
        current = &gResHandleRefCounters->binarySemaphores.at(mHandle);
        *current += count;
    }
    else if constexpr (std::is_same_v<T, Semaphore<SemaphoreType::Timeline>>)
    {
        current = &gResHandleRefCounters->timelineSemaphores.at(mHandle);
        *current += count;
    }
    return *current;
}

template class GRACE_API GpuHandle<Buffer>;
template class GRACE_API GpuHandle<Image>;
template class GRACE_API GpuHandle<Sampler>;
template class GRACE_API GpuHandle<Pipeline<PipelineType::Graphics>>;
template class GRACE_API GpuHandle<Pipeline<PipelineType::Compute>>;
template class GRACE_API GpuHandle<PipelineLayout>;
template class GRACE_API GpuHandle<Fence>;
template class GRACE_API GpuHandle<Semaphore<SemaphoreType::Binary>>;
template class GRACE_API GpuHandle<Semaphore<SemaphoreType::Timeline>>;

} // namespace Grace
