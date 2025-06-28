#include "ResourceManager.hpp"

namespace Grace
{

std::vector<RegistryEntry<Image>>& ResourceManager::GetAllImages()
{
    return m_ImagesRegistry.GetAll();
}

} // namespace Grace
