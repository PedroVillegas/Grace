#pragma once

#include <vector>
#include <queue>

#include <Grace/DeletionQueue.hpp>
#include <Grace/Detail/Assert.hpp>
#include <Grace/GpuBuffer.hpp>
#include <Grace/GpuImage.hpp>
#include <Grace/GpuSampler.hpp>
#include <Grace/GpuPipelineGroup.hpp>
#include <Grace/GpuFence.hpp>
#include <Grace/GpuSemaphore.hpp>
#include <Grace/Detail/Macros.hpp>
#include <Grace/GpuHandle.hpp>

namespace Grace
{

class Device;

template <GpuManaged T>
struct RegistryEntry
{
    template <typename... Args>
    RegistryEntry(const uint32_t Validator, Args&&... args) : validator(Validator), object(std::forward<Args>(args)...)
    {}

    T object = {};
    uint32_t validator = 0U;
};

template <GpuManaged T>
class Registry
{
public:
    ImageHandle Register(Device* device, VkImage image, const GpuImageDesc& desc)
    {
        if (!mFreeSlots.empty())
        {
            GpuHandle<T> newHandle = mFreeSlots.front();
            mFreeSlots.pop();
            mRegistry[newHandle.GetHandle()].validator = newHandle.GetGeneration();
            mRegistry[newHandle.GetHandle()].object = T(device, image, desc);
            return newHandle;
        }

        uint32_t newValidator = mValidator++;
        mRegistry.emplace_back(newValidator, device, image, desc);

        const uint32_t index = static_cast<uint32_t>(mRegistry.size() - 1);
        return GpuHandle<T>(index, newValidator, true);
    }

    GpuHandle<T> Register(Device* device, const GpuObjectDesc<T>& desc, bool refCounted)
    {
        // Use free slots if any available
        if (refCounted)
        {
            if (!mFreeSlots.empty())
            {
                GpuHandle<T> newHandle = mFreeSlots.front();
                mFreeSlots.pop();
                mRegistry[newHandle.GetHandle()].validator = newHandle.GetGeneration();
                mRegistry[newHandle.GetHandle()].object = T(device, desc);
                return newHandle;
            }
        }
        else
        {
            if (!mFreeSlotsNonRefCounted.empty())
            {
                GpuHandle<T> newHandle = mFreeSlotsNonRefCounted.front();
                mFreeSlotsNonRefCounted.pop();
                mRegistry[newHandle.GetHandle()].validator = newHandle.GetGeneration();
                mRegistry[newHandle.GetHandle()].object = T(device, desc);
                return newHandle;
            }
        }

        uint32_t newValidator = mValidator++;
        mRegistry.emplace_back(newValidator, device, desc);

        const uint32_t index = static_cast<uint32_t>(mRegistry.size() - 1);
        return GpuHandle<T>(index, newValidator, refCounted);
    }

    T& Get(const GpuHandle<T>& objectHandle)
    {
        GRACE_ASSERT_MSG(objectHandle.Exists(), "Handle is invalid!");
        const bool handleInRange = objectHandle.GetHandle() < mRegistry.size();
        GRACE_ASSERT_MSG(handleInRange, "Handle is out of bounds!");
        const bool isValidSlot = mRegistry[objectHandle.GetHandle()].validator == objectHandle.GetGeneration();
        GRACE_ASSERT_MSG(isValidSlot, "Handle validator does not match validator of the slot it's in!");

        return mRegistry[objectHandle.GetHandle()].object;
    }

    void Free(GpuHandle<T>& objectHandle, bool deferred)
    {
        GRACE_ASSERT_MSG(objectHandle.Exists(), "Handle is invalid!");
        const bool handleInRange = objectHandle.GetHandle() < mRegistry.size();
        GRACE_ASSERT_MSG(handleInRange, "Handle is out of bounds!");
        const bool isValidSlot = mRegistry[objectHandle.GetHandle()].validator == objectHandle.GetGeneration();
        GRACE_ASSERT_MSG(isValidSlot, "Handle validator does not match validator of the slot it's in!");

        mRegistry[objectHandle.GetHandle()].object = T();
        mRegistry[objectHandle.GetHandle()].validator = INVALID_GENERATION;

        // Slot is freed up and can be reused for the next object created
        if (objectHandle.RefCounted())
        {
            mFreeSlots.emplace(objectHandle.GetHandle(), ++mValidator, true);
        }
        else
        {
            mFreeSlotsNonRefCounted.emplace(objectHandle.GetHandle(), ++mValidator, false);
        }

        // Invalidate objectHandle
        if (!deferred)
        {
            objectHandle = GpuHandle<T>();
        }
    }

private:
    std::vector<RegistryEntry<T>> mRegistry = {};
    std::queue<GpuHandle<T>> mFreeSlots = {};
    std::queue<GpuHandle<T>> mFreeSlotsNonRefCounted = {};
    uint32_t mValidator = 0;
};

/// Handles all graphics objects through registry containers.
///
/// Each `RegistryEntry` of the registry containers has an assigned `validator` which is used
/// to validate the slot against the `GpuHandle` that points to it. If the validation
/// fails, the slot and handle are incorrectly paired so the operation, free or fetch, terminates.
class GpuObjectManager
{
public:
    GpuObjectManager()
    {
        mDeletionQueue = std::make_unique<DeletionQueue>();
    }

    void FlushDeletionQueue(uint32_t frameIndex)
    {
        mDeletionQueue->Flush(frameIndex);
    }

    template <GpuManaged T>
    GRACE_NODISCARD GpuHandle<T> Create(Device* device, const GpuObjectDesc<T>& desc, bool refCounted)
    {
        return ObjectRegistry<T>().Register(device, desc, refCounted);
    }

    GRACE_NODISCARD ImageHandle CreateSwapchainImage(Device* device, VkImage image, const GpuImageDesc& desc);

    template <GpuManaged T>
    GRACE_NODISCARD T& Get(GpuHandle<T> handle)
    {
        return ObjectRegistry<T>().Get(handle);
    }

    template <GpuManaged T>
    void Free(GpuHandle<T>& handle, uint32_t frameIndex = std::numeric_limits<uint32_t>::max())
    {
        if (frameIndex != std::numeric_limits<uint32_t>::max())
        {
            mDeletionQueue->PushDeleter(
                [&] {
                    ObjectRegistry<T>().Free(handle, true);
                },
                frameIndex);
            return;
        }
        ObjectRegistry<T>().Free(handle, false);
    }

private:
    std::unique_ptr<DeletionQueue> mDeletionQueue = nullptr;

    template <GpuManaged T>
    auto& ObjectRegistry();

#define GRACE_DEFINE_GPU_OBJECT_REGISTRY(ObjectType, RegistryName) \
    Registry<ObjectType> RegistryName;                             \
    template <>                                                    \
    auto& ObjectRegistry<ObjectType>()                             \
    {                                                              \
        return RegistryName;                                       \
    };

    GRACE_DEFINE_GPU_OBJECT_REGISTRY(Buffer, mBuffersRegistry);
    GRACE_DEFINE_GPU_OBJECT_REGISTRY(Image, mImagesRegistry);
    GRACE_DEFINE_GPU_OBJECT_REGISTRY(Sampler, mSamplersRegistry);
    GRACE_DEFINE_GPU_OBJECT_REGISTRY(GraphicsPipeline, mGraphicsPipelinesRegistry);
    GRACE_DEFINE_GPU_OBJECT_REGISTRY(ComputePipeline, mComputePipelinesRegistry);
    GRACE_DEFINE_GPU_OBJECT_REGISTRY(PipelineLayout, mPipelineLayoutsRegistry);
    GRACE_DEFINE_GPU_OBJECT_REGISTRY(Fence, mFencesRegistry);
    GRACE_DEFINE_GPU_OBJECT_REGISTRY(BinarySemaphore, mBinarySemaphoresRegistry);
    GRACE_DEFINE_GPU_OBJECT_REGISTRY(TimelineSemaphore, mTimelineSemaphoresRegistry);
};

inline ImageHandle GpuObjectManager::CreateSwapchainImage(Device* device, VkImage image, const GpuImageDesc& desc)
{
    return ObjectRegistry<Image>().Register(device, image, desc);
}

} // namespace Grace
