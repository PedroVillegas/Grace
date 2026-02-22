#include <Private/Grace/StackAllocator.hpp>
#include <Private/Grace/Logging.hpp>

#include <memory>
#include <iostream>
#include <format>

namespace Grace
{

StackAllocatorState& StackAllocatorState::Instance()
{
    thread_local StackAllocatorState instance;
    return instance;
}

StackAllocatorState::~StackAllocatorState()
{
    operator delete(mBuffer);
}

void* StackAllocatorState::Allocate(uint32_t size, uint32_t alignment)
{
    // Bytes
    size_t space = mCapacity - mHead;
    void* ptr = mBuffer + mHead;
    void* alignedptr = std::align(alignment, size, ptr, space);

    if (alignedptr == nullptr || static_cast<std::byte*>(alignedptr) + size > mBuffer + mCapacity)
    {
        GRACE_ERROR("[StackAllocatorState::Allocate] Attempting to allocate {} bytes when {}/{} bytes are actively "
                    "allocated ({} bytes free)",
                    size,
                    mHead,
                    mCapacity,
                    mCapacity - mHead);
        throw std::bad_alloc();
    }

    mHead = static_cast<std::byte*>(alignedptr) - mBuffer + size;
    return alignedptr;
}

void StackAllocatorState::Deallocate(uint32_t size)
{
    mHead -= size;
}

} // namespace Grace
