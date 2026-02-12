#pragma once

#include <functional>
#include <deque>
#include <vector>

namespace Grace
{

class DeletionQueue
{
public:
    ~DeletionQueue() = default;
    DeletionQueue();

    DeletionQueue(const DeletionQueue&) = delete;
    DeletionQueue& operator=(const DeletionQueue&) = delete;

    DeletionQueue(DeletionQueue&& other) noexcept = delete;
    DeletionQueue& operator=(DeletionQueue&& other) noexcept = delete;

    void PushDeleter(std::function<void()>&& deleter, uint32_t frameIndex);

    void Flush(uint32_t frameIndex);

private:
    std::vector<std::deque<std::function<void()>>> mDeleters = {};
};

} // namespace Grace
