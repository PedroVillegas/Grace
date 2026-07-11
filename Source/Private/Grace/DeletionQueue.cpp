#include <Grace/DeletionQueue.hpp>

#include <Private/Grace/Config.hpp>

Grace::DeletionQueue::DeletionQueue()
{
    mDeleters.resize(gConfig.FramesInFlight);
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
