#include "ResourceManager.hpp"

namespace Grace
{

void ResourceManager::FreeAllResources(VkDevice device, VmaAllocator allocator)
{
    m_ImagesRegistry.FreeAll(device, allocator);

    m_BuffersRegistry.FreeAll(allocator);

    m_SamplersRegistry.FreeAll(device);

    m_PipelinesRegistry.FreeAll(device);

    m_PipelineLayoutsRegistry.FreeAll(device);
}

ImageHandle ResourceManager::CreateImage(VkDevice device, VmaAllocator allocator, const ImageDesc& desc)
{
    return m_ImagesRegistry.Register(device, allocator, desc);
}

Image& ResourceManager::GetImage(const ImageHandle& handle)
{
    return m_ImagesRegistry.Get(handle);
}

std::vector<RegistryEntry<Image>>& ResourceManager::GetAllImages()
{
    return m_ImagesRegistry.GetAll();
}

void ResourceManager::FreeImage(VkDevice device, VmaAllocator allocator, ImageHandle& handle)
{
    m_ImagesRegistry.Free(handle, device, allocator);
}

BufferHandle ResourceManager::CreateBuffer(VkDevice device, VmaAllocator allocator, const BufferDesc& desc)
{
    return m_BuffersRegistry.Register(device, allocator, desc);
}

Buffer& ResourceManager::GetBuffer(const BufferHandle& handle)
{
    return m_BuffersRegistry.Get(handle);
}

void ResourceManager::FreeBuffer(VkDevice device, VmaAllocator allocator, BufferHandle& handle)
{
    m_BuffersRegistry.Free(handle, allocator);
}

SamplerHandle ResourceManager::CreateSampler(VkDevice device, const SamplerDesc& desc)
{
    return m_SamplersRegistry.Register(device, desc);
}

Sampler& ResourceManager::GetSampler(const SamplerHandle& handle)
{
    return m_SamplersRegistry.Get(handle);
}

void ResourceManager::FreeSampler(VkDevice device, SamplerHandle& handle)
{
    m_SamplersRegistry.Free(handle, device);
}

PipelineHandle ResourceManager::CreatePipeline(VkDevice device, const PipelineDesc& desc)
{
    return m_PipelinesRegistry.Register(device, desc);
}

Pipeline& ResourceManager::GetPipeline(const PipelineHandle& handle)
{
    return m_PipelinesRegistry.Get(handle);
}

void ResourceManager::FreePipeline(VkDevice device, PipelineHandle& handle)
{
    m_PipelinesRegistry.Free(handle, device);
}

PipelineLayoutHandle ResourceManager::CreatePipelineLayout(VkDevice device, const PipelineLayoutDesc& desc)
{
    return m_PipelineLayoutsRegistry.Register(device, desc);
}

PipelineLayout& ResourceManager::GetPipelineLayout(const PipelineLayoutHandle& handle)
{
    return m_PipelineLayoutsRegistry.Get(handle);
}

void ResourceManager::FreePipelineLayout(VkDevice device, PipelineLayoutHandle& handle)
{
    m_PipelineLayoutsRegistry.Free(handle, device);
}

} // namespace Grace
