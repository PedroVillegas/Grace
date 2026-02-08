#pragma once

#include <Grace/GraceApi.hpp>

#include <cstdint>

namespace std
{
enum class byte : unsigned char;
}

namespace Grace
{

constexpr uint32_t kStackAllocatorInitialSize = 100U * 1024U; // 100 KiB

class GRACE_EXPORT_TESTBED StackAllocatorState
{
public:
    static StackAllocatorState& Instance();

    StackAllocatorState()
        : mBuffer(static_cast<std::byte*>(operator new(kStackAllocatorInitialSize))),
          mCapacity(kStackAllocatorInitialSize)
    {}

    ~StackAllocatorState();

    StackAllocatorState(const StackAllocatorState& other) = delete;
    StackAllocatorState& operator=(const StackAllocatorState& other) = delete;

    StackAllocatorState(StackAllocatorState&& other) = delete;
    StackAllocatorState& operator=(StackAllocatorState&& other) = delete;

    void* Allocate(uint32_t size, uint32_t alignment);
    void Deallocate(uint32_t size);

private:
    std::byte* mBuffer = nullptr;
    uint32_t mHead = 0;
    uint32_t mCapacity = 0;
};

template <typename T>
class GRACE_EXPORT_TESTBED StackAllocator
{
public:
    using value_type = T;

    StackAllocator() = default;

    template <typename U>
    explicit StackAllocator(const StackAllocator<U>& other)
    {}

    T* allocate(size_t n)
    {
        return static_cast<T*>(StackAllocatorState::Instance().Allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T*, size_t n) noexcept
    {
        StackAllocatorState::Instance().Deallocate(n * sizeof(T));
    }

    template <typename U>
    struct rebind
    {
        using other = StackAllocator<U>;
    };
};

template <typename T, typename U>
bool operator==(const StackAllocator<T>& lhs, const StackAllocator<U>& rhs)
{
    return true;
}

template <typename T, typename U>
bool operator!=(const StackAllocator<T>& lhs, const StackAllocator<U>& rhs)
{
    return false;
}

} // namespace Grace
