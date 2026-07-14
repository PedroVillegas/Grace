#pragma once

#include <Grace/GraceApi.hpp>
#include <Grace/Detail/Macros.hpp>
#include <Grace/GpuObjectTraits.hpp>
#include <Grace/GpuObjectFwd.hpp>

#include <cstdint>

namespace Grace
{

constexpr uint32_t INVALID_HANDLE = ~0U;
constexpr uint32_t INVALID_GENERATION = ~0U;

template <bool>
struct RefCountFlag
{
    static constexpr bool value = true;
};

template <>
struct RefCountFlag<false>
{
    static constexpr bool value = false;
};

using DoRefCount = RefCountFlag<true>;
using DontRefCount = RefCountFlag<false>;

template <typename T>
class GRACE_API GpuHandle
{
public:
    GpuHandle() = default;

    GpuHandle(uint32_t UUID, uint32_t Validator, bool RefCounted);

    ~GpuHandle();

    GpuHandle& operator=(const GpuHandle& rhs);
    GpuHandle(const GpuHandle& rhs);

    GpuHandle& operator=(GpuHandle&& rhs) noexcept;
    GpuHandle(GpuHandle&& rhs) noexcept;

    GRACE_NODISCARD bool Exists() const
    {
        return mHandle != INVALID_HANDLE;
    }

    GRACE_NODISCARD bool IsAlive() const
    {
        return ((mGeneration >> 30u) & 0x1) == 0;
    }

    GRACE_NODISCARD bool RefCounted() const noexcept
    {
        return (mGeneration >> 31u) == 1u;
    }

    GRACE_NODISCARD uint32_t GetGeneration() const
    {
        return mGeneration & 0x3fffffff;
    }

    GRACE_NODISCARD uint32_t GetHandle() const
    {
        return mHandle;
    }

    bool operator==(const GpuHandle& rhs) const
    {
        return mHandle == rhs.mHandle;
    }

    bool operator!=(const GpuHandle& rhs) const
    {
        return !(*this == rhs);
    }

private:
    void AbandonHandle() const;
    void InstantiateRefCounter() const;
    uint32_t AdjustRefCounter(int count) const;

private:
    uint32_t mHandle = INVALID_HANDLE;

    // 1 bit for ref counted flag, 1 bit for aliveness, 30 bits for generation
    uint32_t mGeneration = INVALID_GENERATION;
};

} // namespace Grace
