#pragma once

#include <Grace/GpuHandle.hpp>
#include <Grace/TypesVector.hpp>
#include <Grace/Enums.hpp>
#include <Grace/GpuObjectTraits.hpp>
#include <Grace/GpuObjectFwd.hpp>

#include <vk_mem_alloc.h>
#include <initializer_list>
#include <string>

namespace Grace
{

class Device;

template <typename T>
struct GpuObjectDesc
{};

template <>
struct GpuObjectDesc<Buffer>
{
    /// Name used to identify the buffer, e.g. in validation errors
    const char* name;
    /// Specifies how the buffer is allowed to be used
    BufferUsage usage;
    /// Flags used by VMA to optimise buffer allocation
    VmaAllocationCreateFlags allocFlags;
    /// Allocation size in bytes
    size_t size;
    /// Pointer to data used to fill the buffer with upon creation
    const void* data;
};

template <>
struct GpuObjectDesc<Image>
{
    /// Name used to identify the image, e.g. in validation errors
    const char* name;
    /// Specifies the image's dimensions
    UInt3 dimensions;
    /// Specifies the image's format
    Format format;
    /// Specifies how the image is allowed to be used
    ImageUsage usage;
    /// Specifies what access type the image should be initialised for upon creation
    AccessType access = AccessType::None;
    /// Size of data in bytes
    size_t size = 0;
    /// Pointer to data used to fill the image with upon creation
    const void* data = nullptr;
    /// Specifies whether mipmaps should be generated
    bool mipmapped = false;
};

template <>
struct GpuObjectDesc<Sampler>
{
    /// Specifies the minification filter to use when sampling an image
    Filter minFilter;
    /// Specifies the magnification filter to use when sampling an image
    Filter magFilter;
    /// Specifies the behaviour of sampling with image coordinates outside the image
    SamplerAddressMode addressMode;
    /// Specifies the mipmap mode to use when sampling an image
    SamplerMipmapMode mipmapMode;
};

struct GRACE_API ShaderDesc
{
    ShaderStage stage;
    std::string name;
    ShaderFlags flags = ShaderFlags::RelativePath;
};

struct GRACE_API GraphicsState
{
    std::initializer_list<Format> colourAttachmentFormats;
    Format depthAttachmentFormat = Format::Undefined;
    Format stencilAttachmentFormat = Format::Undefined;
    Topology topology = Topology::TriangleList;
    PolygonMode polygonMode = PolygonMode::Fill;
    CullMode cullMode = CullMode::None;
    FrontFace frontFace = FrontFace::Clockwise;
    MultisampleLevel multisample = MultisampleLevel::x1;
    ColorBlendMode colorBlendMode = ColorBlendMode::NoBlend;
    DepthStencilUsage depthStencilUsage = DepthStencilUsage::None;
    CompareOp depthCompareOp = CompareOp::LessOrEqual;
};

template <>
struct GpuObjectDesc<Pipeline<PipelineType::Graphics>>
{
    const char* name = "no_name";
    std::initializer_list<ShaderDesc> shaders;
    GraphicsState graphicsState;
    PipelineLayoutHandle layout;
};

template <>
struct GpuObjectDesc<Pipeline<PipelineType::Compute>>
{
    const char* name = "no_name";
    ShaderDesc shader;
    PipelineLayoutHandle layout;
};

template <>
struct GpuObjectDesc<PipelineLayout>
{
    const char* name;
    VkPipelineLayoutCreateFlags flags;
    std::initializer_list<VkDescriptorSetLayout> setLayouts;
    std::initializer_list<VkPushConstantRange> pushConstantRanges;
};

template <>
struct GpuObjectDesc<Fence>
{
    const char* name = "";
    FenceFlags flags = {};
};

template <>
struct GpuObjectDesc<Semaphore<SemaphoreType::Binary>>
{
    const char* name = "";
    uint64_t initialValue = 0;
};

template <>
struct GpuObjectDesc<Semaphore<SemaphoreType::Timeline>>
{
    const char* name = "";
    uint64_t initialValue = 0;
};

template <typename T>
concept GpuManaged = std::movable<T> && !std::copyable<T> && requires(T r, Device* d, const GpuObjectDesc<T>& desc) {
    T(d, desc);
    { r.Exists() } -> std::same_as<bool>;
    r.VkHandle();
};

using GpuBufferDesc = GpuObjectDesc<Buffer>;
using GpuImageDesc = GpuObjectDesc<Image>;
using GpuSamplerDesc = GpuObjectDesc<Sampler>;
using GpuGraphicsPipelineDesc = GpuObjectDesc<Pipeline<PipelineType::Graphics>>;
using GpuComputePipelineDesc = GpuObjectDesc<Pipeline<PipelineType::Compute>>;
using GpuPipelineLayoutDesc = GpuObjectDesc<PipelineLayout>;
using GpuFenceDesc = GpuObjectDesc<Fence>;
using GpuBinarySemaphoreDesc = GpuObjectDesc<Semaphore<SemaphoreType::Binary>>;
using GpuTimelineSemaphoreDesc = GpuObjectDesc<Semaphore<SemaphoreType::Timeline>>;

} // namespace Grace
