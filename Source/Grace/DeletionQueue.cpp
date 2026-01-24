#include "DeletionQueue.hpp"

Grace::DeletionQueue::DeletionQueue(uint32_t framesInFlight)
{
    mDeleters.resize(framesInFlight);
}

void Grace::DeletionQueue::PushDeleter(std::function<void()>&& deleter, uint32_t frameIndex)
{
    mDeleters[frameIndex].push_back(deleter);
}

void Grace::DeletionQueue::Flush(uint32_t frameIndex)
{
    for (auto it = mDeleters[frameIndex].rbegin(); it != mDeleters[frameIndex].rend(); it++)
    {
        (*it)();
    }
    mDeleters[frameIndex].clear();
}
