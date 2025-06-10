#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Grace
{

/// Description used to create a Sampler object
struct SamplerDesc
{
    /// Specifies the minification filter to use when sampling a image
    VkFilter minFilter;
    /// Specifies the magnification filter to use when sampling a image
    VkFilter magFilter;
    /// Specifies the behavior of sampling with image coordinates outside the image
    VkSamplerAddressMode addressMode;
    /// Specifies the mipmap mode to use when sampling a image
    VkSamplerMipmapMode mipmapMode;
};

/// @brief `VkSampler` objects are required to read image data
/// and apply filtering and other transformations for the shader.
class Sampler
{
public:
    Sampler() = default;
    Sampler(VkDevice device, const SamplerDesc& desc);
    ~Sampler() = default;

    /// Sets index to resource in bindless array of Samplers for access on GPU.
    void SetSamplerId(const uint32_t id);

    /// @returns Index to resource in bindless array of Samplers for access on GPU.
    [[nodiscard]] uint32_t GetSamplerId() const;

    void Create(VkDevice device, const SamplerDesc& desc);

    void Cleanup(VkDevice device);

    [[nodiscard]] VkSampler GetSampler() const;

private:
    VkSampler m_Sampler = {};
    uint32_t m_SamplerId = {};
};

} // namespace Grace
