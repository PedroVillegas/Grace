#include <Grace/GpuPipelineGroup.hpp>
#include <Grace/Device.hpp>
#include <Grace/Detail/ScratchVector.hpp>

namespace Grace
{

PipelineLayout::~PipelineLayout()
{
    if (mPipelineLayout != nullptr)
    {
        vkDestroyPipelineLayout(mDevice->VkHandle(), mPipelineLayout, nullptr);
    }
}

PipelineLayout::PipelineLayout(Device* pDevice, const GpuPipelineLayoutDesc& desc) : mDevice(pDevice)
{
    GRACE_ASSERT(!mDevice->IsNull());

    VkPipelineLayoutCreateInfo plcInfo = {};
    plcInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plcInfo.pNext = nullptr;
    plcInfo.flags = desc.flags;
    plcInfo.setLayoutCount = static_cast<uint32_t>(desc.setLayouts.size());
    plcInfo.pSetLayouts = desc.setLayouts.begin();
    plcInfo.pushConstantRangeCount = static_cast<uint32_t>(desc.pushConstantRanges.size());
    plcInfo.pPushConstantRanges = desc.pushConstantRanges.begin();

    DebugReporter::Check(vkCreatePipelineLayout(mDevice->VkHandle(), &plcInfo, nullptr, &mPipelineLayout));
    AssignDebugName(mDevice->VkHandle(), mPipelineLayout, desc.name);
}

PipelineLayout::PipelineLayout(PipelineLayout&& other) noexcept
    : mDevice(other.mDevice), mPipelineLayout(other.mPipelineLayout)
{
    other.mPipelineLayout = nullptr;
}

PipelineLayout& PipelineLayout::operator=(PipelineLayout&& other) noexcept
{
    if (mPipelineLayout != nullptr)
    {
        vkDestroyPipelineLayout(mDevice->VkHandle(), mPipelineLayout, nullptr);
    }

    mDevice = other.mDevice;
    mPipelineLayout = other.mPipelineLayout;
    other.mPipelineLayout = nullptr;

    return *this;
}

bool PipelineLayout::Exists() const
{
    return mPipelineLayout == nullptr;
}

VkPipelineLayout PipelineLayout::VkHandle() const
{
    return mPipelineLayout;
}

template <PipelineVariant T>
Pipeline<T>::~Pipeline()
{
    if (mPipeline != nullptr)
    {
        vkDestroyPipeline(mDevice->VkHandle(), mPipeline, nullptr);
    }
}

template <>
Pipeline<PipelineType::Graphics>::Pipeline(Device* device, const GpuGraphicsPipelineDesc& desc) : mDevice(device)
{
    GRACE_ASSERT(device != nullptr);

    ScratchVector<VkPipelineShaderStageCreateInfo> shaderStages = {};
    shaderStages.reserve(desc.shaders.size());

    ScratchVector<VkShaderModule> shaderModules = {};
    shaderModules.reserve(desc.shaders.size());

    for (const ShaderDesc& shader : desc.shaders)
    {
        std::filesystem::path filepath = {};
        if (shader.flags == ShaderFlags::RelativePath)
        {
            filepath = GRACE_SPIRV_DIR "/" + shader.name;
        }
        else if (shader.flags == ShaderFlags::AbsolutePath)
        {
            filepath = shader.name;
        }

        GRACE_ASSERT(std::filesystem::exists(filepath));
        GRACE_ASSERT(shader.name.ends_with(".spv"));

        VkShaderModule shaderModule = nullptr;
        CreateShaderModule(device->VkHandle(), filepath, shaderModule);
        shaderStages.push_back(ShaderStageCreateInfo(static_cast<VkShaderStageFlagBits>(shader.stage), shaderModule));
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
        .pColorAttachmentFormats = EnumCastSafe<Format, VkFormat>(desc.graphicsState.colourAttachmentFormats.begin()),
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
        .layout = device->Get<PipelineLayout>(desc.layout).VkHandle(),
        .renderPass = nullptr,
        .subpass = 0,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = 0,
    };

    DebugReporter::Check(vkCreateGraphicsPipelines(device->VkHandle(), nullptr, 1, &gpci, nullptr, &mPipeline));

    for (VkShaderModule module : shaderModules)
    {
        vkDestroyShaderModule(device->VkHandle(), module, nullptr);
    }

    if (mPipeline != nullptr)
    {
        AssignDebugName<VkPipeline>(device->VkHandle(), mPipeline, desc.name);
    }
}

template <>
Pipeline<PipelineType::Compute>::Pipeline(Device* device, const GpuComputePipelineDesc& desc) : mDevice(device)
{
    GRACE_ASSERT(device != nullptr);

    const ShaderDesc& shader = desc.shader;

    const std::filesystem::path filepath = GRACE_SPIRV_DIR "/" + shader.name;

    GRACE_ASSERT(std::filesystem::exists(filepath));
    GRACE_ASSERT(shader.name.ends_with(".spv"));

    VkShaderModule shaderModule = nullptr;
    CreateShaderModule(mDevice->VkHandle(), filepath, shaderModule);
    const VkPipelineShaderStageCreateInfo shaderStageCreateInfo =
        ShaderStageCreateInfo(static_cast<VkShaderStageFlagBits>(shader.stage), shaderModule);

    const VkComputePipelineCreateInfo cpci = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = shaderStageCreateInfo,
        .layout = mDevice->Get<PipelineLayout>(desc.layout).VkHandle(),
        .basePipelineHandle = nullptr,
        .basePipelineIndex = 0,
    };

    DebugReporter::Check(vkCreateComputePipelines(mDevice->VkHandle(), nullptr, 1, &cpci, nullptr, &mPipeline));

    vkDestroyShaderModule(mDevice->VkHandle(), shaderModule, nullptr);

    if (mPipeline != nullptr)
    {
        AssignDebugName<VkPipeline>(mDevice->VkHandle(), mPipeline, desc.name);
    }
}

template <PipelineVariant T>
Pipeline<T>::Pipeline(Pipeline&& other) noexcept : mDevice(other.mDevice), mPipeline(other.mPipeline)
{
    other.mDevice = nullptr;
    other.mPipeline = nullptr;
}

template <PipelineVariant T>
Pipeline<T>& Pipeline<T>::operator=(Pipeline&& other) noexcept
{
    if (mPipeline != nullptr)
    {
        vkDestroyPipeline(mDevice->VkHandle(), mPipeline, nullptr);
    }

    mDevice = other.mDevice;
    mPipeline = other.mPipeline;
    other.mDevice = nullptr;
    other.mPipeline = nullptr;

    return *this;
}

template <PipelineVariant T>
bool Pipeline<T>::Exists() const
{
    return mPipeline == nullptr;
}

template <PipelineVariant T>
VkPipeline Pipeline<T>::VkHandle() const
{
    return mPipeline;
}

template <>
VkPipelineBindPoint Pipeline<PipelineType::Graphics>::BindPoint() const
{
    return VK_PIPELINE_BIND_POINT_GRAPHICS;
}

template <>
VkPipelineBindPoint Pipeline<PipelineType::Compute>::BindPoint() const
{
    return VK_PIPELINE_BIND_POINT_COMPUTE;
}

template class Pipeline<PipelineType::Graphics>;
template class Pipeline<PipelineType::Compute>;

} // namespace Grace
