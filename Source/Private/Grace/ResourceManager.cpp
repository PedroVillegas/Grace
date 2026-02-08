#include <Grace/ResourceManager.hpp>

namespace Grace
{

ResourceManager::ResourceManager(uint32_t framesInFlight)
{
    mDeletionQueue = std::make_unique<DeletionQueue>(framesInFlight);
}

void ResourceManager::FlushDeletionQueue(uint32_t frameIndex)
{
    mDeletionQueue->Flush(frameIndex);
}

std::vector<RegistryEntry<Image>>& ResourceManager::GetAllImages()
{
    return mImagesRegistry.GetAll();
}

} // namespace Grace
