#include <Grace/Sampler.hpp>
#include <Grace/Device.hpp>
#include <Grace/DebugReporter.hpp>
#include <Private/Grace/Assert.hpp>

namespace Grace
{

Sampler::~Sampler()
{
    if (mSampler != nullptr)
    {
        vkDestroySampler(mDevice->GetVkHandle(), mSampler, nullptr);
    }
}

Sampler::Sampler(Device* pDevice, const SamplerDesc& desc) : mDevice(pDevice)
{
    GRACE_ASSERT(!mDevice->IsNull());

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

    DebugReporter::Check(vkCreateSampler(mDevice->GetVkHandle(), &samplerInfo, nullptr, &mSampler));
}

Sampler::Sampler(Sampler&& other) noexcept : mDevice(other.mDevice), mSampler(other.mSampler)
{
    other.mSampler = nullptr;
}

Sampler& Sampler::operator=(Sampler&& other) noexcept
{
    if (mSampler != nullptr)
    {
        vkDestroySampler(mDevice->GetVkHandle(), mSampler, nullptr);
    }

    mDevice = other.mDevice;
    mSampler = other.mSampler;
    other.mSampler = nullptr;

    return *this;
}

bool Sampler::IsNull() const
{
    return mSampler == nullptr;
}

VkSampler Sampler::GetVkHandle() const
{
    return mSampler;
}

void Sampler::SetSamplerId(const uint32_t id)
{
    mSamplerId = id;
}

uint32_t Sampler::GetSamplerId() const
{
    return mSamplerId;
}

} // namespace Grace
