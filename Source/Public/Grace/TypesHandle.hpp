#pragma once

#include <Grace/GraceApi.hpp>
#include <Grace/Macros.hpp>

#include <cstdint>

namespace Grace
{

namespace SemaphoreType
{
struct Binary;
struct Timeline;
} // namespace SemaphoreType

constexpr uint32_t INVALID_HANDLE = ~0U;
constexpr uint32_t INVALID_VALIDATOR = ~0U;

template <typename ResourceType>
class Handle;

GRACE_DEFINE_RESOURCE_HANDLE(Buffer);
GRACE_DEFINE_RESOURCE_HANDLE(Image);
GRACE_DEFINE_RESOURCE_HANDLE(Sampler);
GRACE_DEFINE_RESOURCE_HANDLE(Pipeline);
GRACE_DEFINE_RESOURCE_HANDLE(PipelineLayout);
GRACE_DEFINE_RESOURCE_HANDLE(Fence);
GRACE_DEFINE_TEMPLATED_RESOURCE_HANDLE(Semaphore, SemaphoreType::Binary, BinarySemaphore);
GRACE_DEFINE_TEMPLATED_RESOURCE_HANDLE(Semaphore, SemaphoreType::Timeline, TimelineSemaphore);

template <typename ResourceType>
class GRACE_API Handle
{
public:
    Handle() = default;

    Handle(uint32_t UUID, uint32_t Validator);

    ~Handle();

    Handle& operator=(const Handle& rhs);
    Handle(const Handle& rhs);

    Handle& operator=(Handle&& rhs) noexcept;
    Handle(Handle&& rhs) noexcept;

    GRACE_NODISCARD bool HasValidHandle() const
    {
        return mHandle != INVALID_HANDLE;
    }

    GRACE_NODISCARD bool IsAlive() const
    {
        return (mValidator31Alive1 & 0x1) == 0;
    }

    GRACE_NODISCARD uint32_t GetValidator() const
    {
        return mValidator31Alive1 >> 1U;
    }

    GRACE_NODISCARD uint32_t GetHandle() const
    {
        return mHandle;
    }

private:
    void AbandonHandle() const;
    void InstantiateRefCounter() const;
    uint32_t AdjustRefCounter(int count) const;

private:
    uint32_t mHandle = INVALID_HANDLE;
    uint32_t mValidator31Alive1 = INVALID_VALIDATOR;
};

} // namespace Grace
