#pragma once

#include <Grace/GpuResourceHandles.hpp>
#include <Grace/TypesVector.hpp>
#include <Grace/Enums.hpp>

#include <vk_mem_alloc.h>
#include <initializer_list>
#include <string>

namespace Grace
{

namespace SemaphoreType
{

struct Binary;
struct Timeline;

} // namespace SemaphoreType

namespace PipelineType
{

struct Graphics;
struct Compute;

}

class Buffer;
class Image;
class Sampler;

template <typename T>
class Pipeline;

class PipelineLayout;
class Fence;

template <typename T>
class Semaphore;

template <typename T>
struct GpuResourceDesc
{};

template <>
struct GpuResourceDesc<Buffer>
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

/*
 *  Use concepts to make a gpu resource interface (constructor: Device*, const ResDesc&)
 *
 *  replace resource manager's args forwarding with concept
 *  template <GpuManageable T>
 *  Registry(device*, GpuResourceDesc<T>) {
 *      ... T(device*, GpuResourceDesc<T>.desc)
 *  }
 *
 */

template <>
struct GpuResourceDesc<Image>
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
struct GpuResourceDesc<Sampler>
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
struct GpuResourceDesc<Pipeline<PipelineType::Graphics>>
{
    const char* name = "no_name";
    std::initializer_list<ShaderDesc> shaders;
    GraphicsState graphicsState;
    PipelineLayoutHandle layout;
};

template <>
struct GpuResourceDesc<Pipeline<PipelineType::Compute>>
{
    const char* name = "no_name";
    ShaderDesc shader;
    PipelineLayoutHandle layout;
};

template <>
struct GpuResourceDesc<PipelineLayout>
{
    const char* name;
    VkPipelineLayoutCreateFlags flags;
    std::initializer_list<VkDescriptorSetLayout> setLayouts;
    std::initializer_list<VkPushConstantRange> pushConstantRanges;
};

template <>
struct GpuResourceDesc<Fence>
{
    const char* name = "";
    FenceFlags flags = {};
};

template <>
struct GpuResourceDesc<Semaphore<SemaphoreType::Binary>>
{
    const char* name = "";
    uint64_t initialValue = 0;
};

template <>
struct GpuResourceDesc<Semaphore<SemaphoreType::Timeline>>
{
    const char* name = "";
    uint64_t initialValue = 0;
};

using GpuBufferDesc = GpuResourceDesc<Buffer>;
using GpuImageDesc = GpuResourceDesc<Image>;
using GpuSamplerDesc = GpuResourceDesc<Sampler>;
using GpuGraphicsPipelineDesc = GpuResourceDesc<Pipeline<PipelineType::Graphics>>;
using GpuComputePipelineDesc = GpuResourceDesc<Pipeline<PipelineType::Compute>>;
using GpuPipelineLayoutDesc = GpuResourceDesc<PipelineLayout>;
using GpuFenceDesc = GpuResourceDesc<Fence>;
using GpuBinarySemaphoreDesc = GpuResourceDesc<Semaphore<SemaphoreType::Binary>>;
using GpuTimelineSemaphoreDesc = GpuResourceDesc<Semaphore<SemaphoreType::Timeline>>;

} // namespace Grace
