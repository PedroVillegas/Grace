#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include <Grace/GraceExport.h>
#include <Grace/Macros.hpp>
#include <Grace/TypesHandle.hpp>
#include <Grace/Enums.hpp>

namespace Grace
{

class Device;

enum class PipelineType : uint8_t
{
    Compute,
    Graphics,
    Undefined
};

struct GRACE_EXPORT PipelineLayoutDesc
{
    const char* name;
    VkPipelineLayoutCreateFlags flags;
    std::vector<VkDescriptorSetLayout> setLayouts;
    std::vector<VkPushConstantRange> pushConstantRanges;
};

class GRACE_EXPORT PipelineLayout
{
public:
    ~PipelineLayout();
    PipelineLayout() = default;
    PipelineLayout(Device* pDevice, const PipelineLayoutDesc& desc);

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkPipelineLayout handle more than once
    PipelineLayout(const PipelineLayout&) = delete;
    PipelineLayout& operator=(const PipelineLayout&) = delete;

    PipelineLayout(PipelineLayout&& other) noexcept;
    PipelineLayout& operator=(PipelineLayout&& other) noexcept;

    GRACE_NODISCARD bool IsNull() const;

    GRACE_NODISCARD VkPipelineLayout GetVkPipelineLayout() const;

private:
    Device* mDevice = nullptr;
    VkPipelineLayout mPipelineLayout = nullptr;
};

struct ShaderDesc
{
    ShaderStage stage;
    std::string name;
};

struct GraphicsState
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

/// Description used to create a Graphics Pipeline object
struct GRACE_EXPORT GraphicsPipelineDesc
{
    const char* name = "no_name";
    std::initializer_list<ShaderDesc> shaders;
    GraphicsState graphicsState;
    PipelineLayoutHandle layout;
};

/// Description used to create a Compute Pipeline object
struct GRACE_EXPORT ComputePipelineDesc
{
    const char* name = "no_name";
    ShaderDesc shader;
    PipelineLayoutHandle layout;
};

class GRACE_EXPORT Pipeline
{
public:
    ~Pipeline();
    Pipeline() = default;
    Pipeline(Device* pDevice, const GraphicsPipelineDesc&& desc);
    Pipeline(Device* pDevice, const ComputePipelineDesc& desc);

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkPipeline handle more than once
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    Pipeline(Pipeline&& other) noexcept;
    Pipeline& operator=(Pipeline&& other) noexcept;

    GRACE_NODISCARD bool IsNull() const;

    GRACE_NODISCARD VkPipeline GetVkHandle() const;

    GRACE_NODISCARD VkPipelineBindPoint BindPoint() const;

private:
    Device* mDevice = nullptr;
    VkPipeline mPipeline = nullptr;
    PipelineType mType = PipelineType::Undefined;
};

} // namespace Grace
