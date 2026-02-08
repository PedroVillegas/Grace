#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <cassert>
#include <array>

#include <Grace/GraceApi.hpp>
#include <Grace/Macros.hpp>
#include <Grace/TypesHandle.hpp>
#include <Grace/Enums.hpp>
#include <Grace/DebugReporter.hpp>
#include <Grace/HelperFunctions.hpp>

namespace Grace
{

class Device;

enum class PipelineType : uint8_t
{
    Compute,
    Graphics,
    Undefined
};

struct GRACE_API PipelineLayoutDesc
{
    const char* name;
    VkPipelineLayoutCreateFlags flags;
    std::initializer_list<VkDescriptorSetLayout> setLayouts;
    std::initializer_list<VkPushConstantRange> pushConstantRanges;
};

class GRACE_API PipelineLayout
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

/// Description used to create a Graphics Pipeline object
struct GRACE_API GraphicsPipelineDesc
{
    const char* name = "no_name";
    std::initializer_list<ShaderDesc> shaders;
    GraphicsState graphicsState;
    PipelineLayoutHandle layout;
};

/// Description used to create a Compute Pipeline object
struct GRACE_API ComputePipelineDesc
{
    const char* name = "no_name";
    ShaderDesc shader;
    PipelineLayoutHandle layout;
};

class GRACE_API Pipeline
{
public:
    ~Pipeline();
    Pipeline() = default;

    Pipeline(VkDevice device, const PipelineLayout& pl, const GraphicsPipelineDesc&& desc);
    Pipeline(VkDevice device, const PipelineLayout& pl, const ComputePipelineDesc& desc);

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
    VkDevice mDevice = nullptr;
    VkPipeline mPipeline = nullptr;
    PipelineType mType = PipelineType::Undefined;
};

} // namespace Grace
