#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <cassert>
#include <array>

#include <Grace/GraceExport.h>
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

template <size_t NumColourAttachments>
struct GraphicsState
{
    std::array<Format, NumColourAttachments> colourAttachmentFormats = {};
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
template <size_t NumShaders, size_t NumColourAttachments>
struct GRACE_EXPORT GraphicsPipelineDesc
{
    const char* name = "no_name";
    std::array<ShaderDesc, NumShaders> shaders;
    GraphicsState<NumColourAttachments> graphicsState;
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

    template <size_t NumShaders, size_t NumColourAttachments>
    Pipeline(VkDevice device,
             const PipelineLayout& pl,
             const GraphicsPipelineDesc<NumShaders, NumColourAttachments>&& desc);
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

template <size_t NumShaders, size_t NumColourAttachments>
Pipeline::Pipeline(VkDevice device,
                   const PipelineLayout& pl,
                   const GraphicsPipelineDesc<NumShaders, NumColourAttachments>&& desc)
    : mDevice(device), mType(PipelineType::Graphics)
{
    assert(device != nullptr);

    std::array<VkPipelineShaderStageCreateInfo, NumShaders> shaderStages = {};
    std::array<VkShaderModule, NumShaders> shaderModules = {};

    for (int i = 0; i < NumShaders; i++)
    {
        const ShaderDesc& shader = desc.shaders[i];
        const std::filesystem::path filepath = GRACE_SPIRV_DIR "/" + shader.name;

        assert(std::filesystem::exists(filepath));
        assert(shader.name.ends_with(".spv"));

        VkShaderModule shaderModule = nullptr;
        CreateShaderModule(device, filepath, shaderModule);
        shaderStages[i] = ShaderStageCreateInfo(static_cast<VkShaderStageFlagBits>(shader.stage), shaderModule);
        shaderModules[i] = shaderModule;
    }

    const VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };

    const VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = static_cast<VkPolygonMode>(desc.graphicsState.polygonMode),
        .cullMode = static_cast<VkCullModeFlags>(desc.graphicsState.cullMode),
        .frontFace = static_cast<VkFrontFace>(desc.graphicsState.frontFace),
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0F,
        .depthBiasClamp = 0.0F,
        .depthBiasSlopeFactor = 0.0F,
        .lineWidth = 1.0F,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    switch (desc.graphicsState.colorBlendMode)
    {
    case ColorBlendMode::Additive:
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        break;
    case ColorBlendMode::AlphaBlend:
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        break;
    default:
        break;
    }

    const VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = static_cast<VkLogicOp>(LogicOp::Copy),
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = { 0.0F, 0.0F, 0.0F, 0.0F },
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = static_cast<VkCompareOp>(desc.graphicsState.depthCompareOp),
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0F,
        .maxDepthBounds = 1.0F,
    };

    // TODO: handle stencil test and depth bounds test
    switch (desc.graphicsState.depthStencilUsage)
    {
    case DepthStencilUsage::DepthTestNoWrite:
        depthStencilState.depthTestEnable = VK_TRUE;
        break;
    case DepthStencilUsage::DepthOnly:
        depthStencilState.depthTestEnable = VK_TRUE;
        depthStencilState.depthWriteEnable = VK_TRUE;
        break;
    case DepthStencilUsage::StencilOnly:
        depthStencilState.stencilTestEnable = VK_TRUE;
        break;
    case DepthStencilUsage::All:
        depthStencilState.depthTestEnable = VK_TRUE;
        depthStencilState.depthWriteEnable = VK_TRUE;
        depthStencilState.stencilTestEnable = VK_TRUE;
        break;
    default:
        break;
    }

    const VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = static_cast<VkSampleCountFlagBits>(desc.graphicsState.multisample),
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0F,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    std::array ds = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    const VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<uint32_t>(ds.size()),
        .pDynamicStates = ds.data(),
    };

    const VkPipelineRenderingCreateInfo rendering = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = static_cast<uint32_t>(desc.graphicsState.colourAttachmentFormats.size()),
        .pColorAttachmentFormats = EnumCastSafe<Format, VkFormat>(desc.graphicsState.colourAttachmentFormats.data()),
        .depthAttachmentFormat = static_cast<VkFormat>(desc.graphicsState.depthAttachmentFormat),
        .stencilAttachmentFormat = static_cast<VkFormat>(desc.graphicsState.stencilAttachmentFormat),
    };

    constexpr VkPipelineVertexInputStateCreateInfo vertexInputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
    };

    constexpr VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false,
    };

    constexpr VkPipelineTessellationStateCreateInfo tessellationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .patchControlPoints = 0,
    };

    // TODO: handle vertex input state, input assembly state and tessellation state
    const VkGraphicsPipelineCreateInfo gpci = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering,
        .flags = 0,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pTessellationState = &tessellationState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = pl.GetVkPipelineLayout(),
        .renderPass = nullptr,
        .subpass = 0,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = 0,
    };

    DebugReporter::Check(vkCreateGraphicsPipelines(device, nullptr, 1, &gpci, nullptr, &mPipeline));

    for (VkShaderModule module : shaderModules)
    {
        vkDestroyShaderModule(device, module, nullptr);
    }

    if (mPipeline != nullptr)
    {
        AssignDebugName<VkPipeline>(device, mPipeline, desc.name);
    }
}

} // namespace Grace
