#pragma once

#include <vector>
#include <queue>
#include <cassert>

#include <Private/Grace/DeletionQueue.hpp>
#include <Grace/Buffer.hpp>
#include <Grace/Image.hpp>
#include <Grace/Sampler.hpp>
#include <Grace/PipelineGroup.hpp>
#include <Grace/Fence.hpp>
#include <Grace/Semaphore.hpp>
#include <Grace/TypesHandle.hpp>
#include <Grace/Macros.hpp>

namespace Grace
{

class Device;

template <typename Res>
struct RegistryEntry
{
    template <typename... Args>
    RegistryEntry(const uint32_t Validator, Args&&... args)
        : validator(Validator), resource(std::forward<Args>(args)...)
    {}

    Res resource = {};
    uint32_t validator = 0U;
};

template <typename Res>
class Registry
{
public:
    template <typename... Args>
    Handle<Res> Register(Args&&... args)
    {
        // Use free slots if any available
        if (!mFreeSlots.empty())
        {
            Handle<Res> newHandle = mFreeSlots.front();
            mFreeSlots.pop();
            mRegistry[newHandle.GetHandle()].validator = newHandle.GetValidator();
            mRegistry[newHandle.GetHandle()].resource = Res(std::forward<Args>(args)...);
            return newHandle;
        }

        uint32_t newValidator = mValidator++;
        mRegistry.emplace_back(newValidator, std::forward<Args>(args)...);

        const uint32_t index = static_cast<uint32_t>(mRegistry.size() - 1);
        return Handle<Res>(index, newValidator << 1);
    }

    Res& Get(const Handle<Res>& resourceHandle)
    {
        assert(resourceHandle.HasValidHandle() && "Handle is invalid.");
        const bool handleInRange = resourceHandle.GetHandle() < mRegistry.size();
        assert(handleInRange && "Handle is not in range.");
        const bool isValidSlot = mRegistry[resourceHandle.GetHandle()].validator == resourceHandle.GetValidator();
        assert(isValidSlot && "Handle validator does not match validator of the slot it's in.");

        return mRegistry[resourceHandle.GetHandle()].resource;
    }

    void Free(Handle<Res>& resourceHandle, bool deferred)
    {
        assert(resourceHandle.HasValidHandle() && "Handle is invalid.");
        const bool handleInRange = resourceHandle.GetHandle() < mRegistry.size();
        assert(handleInRange && "Handle is not in range.");
        const bool isValidSlot = mRegistry[resourceHandle.GetHandle()].validator == resourceHandle.GetValidator();
        assert(isValidSlot && "Handle validator does not match validator of the slot it's in.");

        mRegistry[resourceHandle.GetHandle()].resource = Res();
        mRegistry[resourceHandle.GetHandle()].validator = INVALID_VALIDATOR;

        // Slot is freed up and can be reused for the next resource created
        mFreeSlots.emplace(resourceHandle.GetHandle(), ++mValidator);

        // Invalidate resourceHandle
        if (!deferred)
        {
            resourceHandle = Handle<Res>();
        }
    }

private:
    std::vector<RegistryEntry<Res>> mRegistry = {};
    std::queue<Handle<Res>> mFreeSlots = {};
    uint32_t mValidator = 0;
};

/// Handles all graphics resources through registry containers.
///
/// Each `RegistryEntry` of the registry containers has an assigned `validator` which is used
/// to validate the slot against the `ResourceHandle` that points to it. If the validation
/// fails, the slot and handle are incorrectly paired so the operation, free or fetch, terminates.
class ResourceManager
{
public:
    ResourceManager()
    {
        mDeletionQueue = std::make_unique<DeletionQueue>();
    }

    void FlushDeletionQueue(uint32_t frameIndex)
    {
        mDeletionQueue->Flush(frameIndex);
    }

    template <typename Res, typename... Args>
    GRACE_NODISCARD Handle<Res> Create(Args&&... args)
    {
        return ResourceRegistry<Res>().Register(std::forward<Args>(args)...);
    }

    template <typename Res>
    GRACE_NODISCARD Res& Get(Handle<Res> handle)
    {
        return ResourceRegistry<Res>().Get(handle);
    }

    template <typename Res>
    void Free(Handle<Res>& handle, uint32_t frameIndex = std::numeric_limits<uint32_t>::max())
    {
        if (frameIndex != std::numeric_limits<uint32_t>::max())
        {
            mDeletionQueue->PushDeleter(
                [&] {
                    ResourceRegistry<Res>().Free(handle, true);
                },
                frameIndex);
            return;
        }
        ResourceRegistry<Res>().Free(handle, false);
    }

private:
    std::unique_ptr<DeletionQueue> mDeletionQueue = nullptr;

    template <typename Res>
    auto& ResourceRegistry();

    GRACE_DEFINE_RESOURCE_REGISTRY(Image, mImagesRegistry);
    GRACE_DEFINE_RESOURCE_REGISTRY(Buffer, mBuffersRegistry);
    GRACE_DEFINE_RESOURCE_REGISTRY(Sampler, mSamplersRegistry);
    GRACE_DEFINE_RESOURCE_REGISTRY(Pipeline, mPipelinesRegistry);
    GRACE_DEFINE_RESOURCE_REGISTRY(PipelineLayout, mPipelineLayoutsRegistry);
    GRACE_DEFINE_RESOURCE_REGISTRY(Fence, mFencesRegistry);
    GRACE_DEFINE_RESOURCE_REGISTRY(BinarySemaphore, mBinarySemaphoresRegistry);
    GRACE_DEFINE_RESOURCE_REGISTRY(TimelineSemaphore, mTimelineSemaphoresRegistry);
};

} // namespace Grace
