#include "Sampler.hpp"

#include <cassert>

#include <Grace/DebugReporter.hpp>

namespace Grace
{

Sampler::Sampler(VkDevice device, const SamplerDesc& desc)
{
    Create(device, desc);
}

VkSampler Sampler::GetSampler() const
{
    return m_Sampler;
}

void Sampler::SetSamplerId(const uint32_t id)
{
    m_SamplerId = id;
}

uint32_t Sampler::GetSamplerId() const
{
    return m_SamplerId;
}

void Sampler::Create(VkDevice device, const SamplerDesc& desc)
{
    assert(device != nullptr);

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.pNext = nullptr;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.minLod = 0;
    samplerInfo.minFilter = desc.minFilter;
    samplerInfo.magFilter = desc.magFilter;
    samplerInfo.mipmapMode = desc.mipmapMode;
    samplerInfo.addressModeU = desc.addressMode;
    samplerInfo.addressModeV = desc.addressMode;
    samplerInfo.addressModeW = desc.addressMode;

    DebugReporter::Check(vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler));
}

void Sampler::Cleanup(VkDevice device)
{
    vkDestroySampler(device, m_Sampler, nullptr);
}

} // namespace Grace
