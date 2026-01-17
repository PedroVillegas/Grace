#include "Sampler.hpp"

#include <cassert>

#include <Grace/Device.hpp>
#include <Grace/DebugReporter.hpp>

namespace Grace
{

Sampler::~Sampler()
{
    if (m_Sampler != nullptr)
    {
        vkDestroySampler(m_Device->GetVkHandle(), m_Sampler, nullptr);
    }
}

Sampler::Sampler(Device* pDevice, const SamplerDesc& desc) : m_Device(pDevice)
{
    assert(!m_Device->IsNull());

    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = static_cast<VkFilter>(desc.magFilter),
        .minFilter = static_cast<VkFilter>(desc.minFilter),
        .mipmapMode = static_cast<VkSamplerMipmapMode>(desc.mipmapMode),
        .addressModeU = static_cast<VkSamplerAddressMode>(desc.addressMode),
        .addressModeV = static_cast<VkSamplerAddressMode>(desc.addressMode),
        .addressModeW = static_cast<VkSamplerAddressMode>(desc.addressMode),
        .mipLodBias = 0,
        .anisotropyEnable = false,
        .maxAnisotropy = 1.0F,
        .compareEnable = false,
        .compareOp = static_cast<VkCompareOp>(CompareOp::Always),
        .minLod = 0,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = static_cast<VkBorderColor>(BorderColor::FloatOpaqueBlack),
        .unnormalizedCoordinates = false,
    };

    DebugReporter::Check(vkCreateSampler(m_Device->GetVkHandle(), &samplerInfo, nullptr, &m_Sampler));
}

Sampler::Sampler(Sampler&& other) noexcept : m_Device(other.m_Device), m_Sampler(other.m_Sampler)
{
    other.m_Sampler = nullptr;
}

Sampler& Sampler::operator=(Sampler&& other) noexcept
{
    if (m_Sampler != nullptr)
    {
        vkDestroySampler(m_Device->GetVkHandle(), m_Sampler, nullptr);
    }

    m_Device = other.m_Device;
    m_Sampler = other.m_Sampler;
    other.m_Sampler = nullptr;

    return *this;
}

bool Sampler::IsNull() const
{
    return m_Sampler == nullptr;
}

VkSampler Sampler::GetVkHandle() const
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

} // namespace Grace
