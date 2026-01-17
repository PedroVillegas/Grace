#include "PipelineGroup.hpp"

#include <iostream>
#include <cassert>

#include <Grace/DebugReporter.hpp>
#include <Grace/Context.hpp>
#include <Grace/HelperFunctions.hpp>

namespace Grace
{

PipelineLayout::~PipelineLayout()
{
    if (m_PipelineLayout != nullptr)
    {
        vkDestroyPipelineLayout(m_Device->GetVkHandle(), m_PipelineLayout, nullptr);
    }
}

PipelineLayout::PipelineLayout(Device* pDevice, const PipelineLayoutDesc& desc) : m_Device(pDevice)
{
    assert(!m_Device->IsNull());

    VkPipelineLayoutCreateInfo plcInfo = {};
    plcInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plcInfo.pNext = nullptr;
    plcInfo.flags = desc.flags;
    plcInfo.setLayoutCount = static_cast<uint32_t>(desc.setLayouts.size());
    plcInfo.pSetLayouts = desc.setLayouts.data();
    plcInfo.pushConstantRangeCount = static_cast<uint32_t>(desc.pushConstantRanges.size());
    plcInfo.pPushConstantRanges = desc.pushConstantRanges.data();

    DebugReporter::Check(vkCreatePipelineLayout(m_Device->GetVkHandle(), &plcInfo, nullptr, &m_PipelineLayout));
    AssignDebugName(m_Device->GetVkHandle(), m_PipelineLayout, desc.name);
}

PipelineLayout::PipelineLayout(PipelineLayout&& other) noexcept
    : m_Device(other.m_Device), m_PipelineLayout(other.m_PipelineLayout)
{
    other.m_PipelineLayout = nullptr;
}

PipelineLayout& PipelineLayout::operator=(PipelineLayout&& other) noexcept
{
    if (m_PipelineLayout != nullptr)
    {
        vkDestroyPipelineLayout(m_Device->GetVkHandle(), m_PipelineLayout, nullptr);
    }

    m_Device = other.m_Device;
    m_PipelineLayout = other.m_PipelineLayout;
    other.m_PipelineLayout = nullptr;

    return *this;
}

bool PipelineLayout::IsNull() const
{
    return m_PipelineLayout == nullptr;
}

VkPipelineLayout PipelineLayout::GetVkPipelineLayout() const
{
    return m_PipelineLayout;
}

Pipeline::~Pipeline()
{
    if (m_Pipeline != nullptr)
    {
        vkDestroyPipeline(m_Device->GetVkHandle(), m_Pipeline, nullptr);
    }
}

Pipeline::Pipeline(Device* pDevice, const PipelineDesc& desc) : m_Device(pDevice)
{
    assert(!pDevice->IsNull());

    if (!desc.shaders.empty())
    {
        m_Type = PipelineType::Graphics;

        const PipelineLayout& pl = pDevice->GetPipelineLayout(desc.layout);
        assert(!pl.IsNull());

        if (desc.graphicsState.colourAttachmentFormats.empty()
            && desc.graphicsState.depthAttachmentFormat == Format::Undefined
            && desc.graphicsState.stencilAttachmentFormat == Format::Undefined)
        {
            m_Type = PipelineType::Compute;
            assert(desc.shaders.size() == 1);

            const ShaderDesc& shader = desc.shaders.at(0);

            const std::filesystem::path filepath = GRACE_SPIRV_DIR "/" + shader.name;

            assert(std::filesystem::exists(filepath));
            assert(shader.name.ends_with(".spv"));

            VkShaderModule shaderModule = nullptr;
            CreateShaderModule(pDevice->GetVkHandle(), filepath, shaderModule);
            const VkPipelineShaderStageCreateInfo shaderStageCreateInfo =
                ShaderStageCreateInfo(static_cast<VkShaderStageFlagBits>(shader.stage), shaderModule);

            const VkComputePipelineCreateInfo cpci = {
                .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = shaderStageCreateInfo,
                .layout = pl.GetVkPipelineLayout(),
                .basePipelineHandle = nullptr,
                .basePipelineIndex = 0,
            };

            DebugReporter::Check(
                vkCreateComputePipelines(pDevice->GetVkHandle(), nullptr, 1, &cpci, nullptr, &m_Pipeline));

            vkDestroyShaderModule(pDevice->GetVkHandle(), shaderModule, nullptr);

            if (m_Pipeline != nullptr)
            {
                AssignDebugName<VkPipeline>(pDevice->GetVkHandle(), m_Pipeline, desc.name);
            }

            return;
        }

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {};
        shaderStages.reserve(desc.shaders.size());

        std::vector<VkShaderModule> shaderModules = {};
        shaderModules.reserve(desc.shaders.size());

        for (const ShaderDesc& shader : desc.shaders)
        {
            const std::filesystem::path filepath = GRACE_SPIRV_DIR "/" + shader.name;

            assert(std::filesystem::exists(filepath));
            assert(shader.name.ends_with(".spv"));

            VkShaderModule shaderModule = nullptr;
            CreateShaderModule(pDevice->GetVkHandle(), filepath, shaderModule);
            shaderStages.push_back(
                ShaderStageCreateInfo(static_cast<VkShaderStageFlagBits>(shader.stage), shaderModule));

            shaderModules.push_back(shaderModule);
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
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                            | VK_COLOR_COMPONENT_A_BIT,
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
            .dynamicStateCount = 2,
            .pDynamicStates = ds.data()
        };

        std::vector<VkFormat> vkformats = {};
        vkformats.reserve(desc.graphicsState.colourAttachmentFormats.size());
        for (const auto& format : desc.graphicsState.colourAttachmentFormats)
        {
            vkformats.push_back(static_cast<VkFormat>(format));
        }

        const VkPipelineRenderingCreateInfo rendering = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = static_cast<uint32_t>(desc.graphicsState.colourAttachmentFormats.size()),
            .pColorAttachmentFormats = vkformats.data(),
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

        DebugReporter::Check(
            vkCreateGraphicsPipelines(pDevice->GetVkHandle(), nullptr, 1, &gpci, nullptr, &m_Pipeline));

        for (VkShaderModule module : shaderModules)
        {
            vkDestroyShaderModule(pDevice->GetVkHandle(), module, nullptr);
        }

        if (m_Pipeline != nullptr)
        {
            AssignDebugName<VkPipeline>(pDevice->GetVkHandle(), m_Pipeline, desc.name);
        }
    }
}

Pipeline::Pipeline(Pipeline&& other) noexcept
    : m_Device(other.m_Device), m_Pipeline(other.m_Pipeline), m_Type(other.m_Type)
{
    other.m_Device = nullptr;
    other.m_Pipeline = nullptr;
    other.m_Type = PipelineType::Undefined;
}

Pipeline& Pipeline::operator=(Pipeline&& other) noexcept
{
    if (m_Pipeline != nullptr)
    {
        vkDestroyPipeline(m_Device->GetVkHandle(), m_Pipeline, nullptr);
    }

    m_Device = other.m_Device;
    m_Pipeline = other.m_Pipeline;
    m_Type = other.m_Type;
    other.m_Device = nullptr;
    other.m_Pipeline = nullptr;
    other.m_Type = PipelineType::Undefined;

    return *this;
}

bool Pipeline::IsNull() const
{
    return m_Pipeline == nullptr;
}

VkPipeline Pipeline::GetVkHandle() const
{
    return m_Pipeline;
}

VkPipelineBindPoint Pipeline::BindPoint() const
{
    switch (m_Type)
    {
    case PipelineType::Compute:
        return VK_PIPELINE_BIND_POINT_COMPUTE;
    case PipelineType::Graphics:
        return VK_PIPELINE_BIND_POINT_GRAPHICS;
    default:
        assert(false);
        return VK_PIPELINE_BIND_POINT_MAX_ENUM;
    }
}

} // namespace Grace
