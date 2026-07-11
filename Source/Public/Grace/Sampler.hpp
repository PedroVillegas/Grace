#pragma once

#include <Grace/GraceApi.hpp>
#include <Grace/Macros.hpp>
#include <Grace/GpuResourceDescriptions.hpp>
#include <Grace/GpuResourceTraits.hpp>

namespace Grace
{

class Device;

/// @brief `VkSampler` objects are required to read image data
/// and apply filtering and other transformations for the shader.
class GRACE_API Sampler : GpuBindlessCompatibleTag
{
public:
    ~Sampler();
    Sampler() = default;
    Sampler(Device* pDevice, const GpuSamplerDesc& desc);

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkSampler handle more than once
    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    Sampler(Sampler&& other) noexcept;
    Sampler& operator=(Sampler&& other) noexcept;

    GRACE_NODISCARD bool Exists() const;

    GRACE_NODISCARD VkSampler VkHandle() const;

    /// Sets index to resource in bindless array of Samplers for access on GPU.
    void SetSamplerId(const uint32_t id);

    /// @returns Index to resource in bindless array of Samplers for access on GPU.
    GRACE_NODISCARD uint32_t GetSamplerId() const;

private:
    Device* mDevice = nullptr;
    VkSampler mSampler = nullptr;
    uint32_t mSamplerId = 0;
};

} // namespace Grace
