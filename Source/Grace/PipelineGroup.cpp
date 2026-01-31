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
    if (mPipelineLayout != nullptr)
    {
        vkDestroyPipelineLayout(mDevice->GetVkHandle(), mPipelineLayout, nullptr);
    }
}

PipelineLayout::PipelineLayout(Device* pDevice, const PipelineLayoutDesc& desc) : mDevice(pDevice)
{
    assert(!mDevice->IsNull());

    VkPipelineLayoutCreateInfo plcInfo = {};
    plcInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plcInfo.pNext = nullptr;
    plcInfo.flags = desc.flags;
    plcInfo.setLayoutCount = static_cast<uint32_t>(desc.setLayouts.size());
    plcInfo.pSetLayouts = desc.setLayouts.data();
    plcInfo.pushConstantRangeCount = static_cast<uint32_t>(desc.pushConstantRanges.size());
    plcInfo.pPushConstantRanges = desc.pushConstantRanges.data();

    DebugReporter::Check(vkCreatePipelineLayout(mDevice->GetVkHandle(), &plcInfo, nullptr, &mPipelineLayout));
    AssignDebugName(mDevice->GetVkHandle(), mPipelineLayout, desc.name);
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
        vkDestroyPipelineLayout(mDevice->GetVkHandle(), mPipelineLayout, nullptr);
    }

    mDevice = other.mDevice;
    mPipelineLayout = other.mPipelineLayout;
    other.mPipelineLayout = nullptr;

    return *this;
}

bool PipelineLayout::IsNull() const
{
    return mPipelineLayout == nullptr;
}

VkPipelineLayout PipelineLayout::GetVkPipelineLayout() const
{
    return mPipelineLayout;
}

Pipeline::~Pipeline()
{
    if (mPipeline != nullptr)
    {
        vkDestroyPipeline(mDevice, mPipeline, nullptr);
    }
}

Pipeline::Pipeline(VkDevice device, const PipelineLayout& pl, const ComputePipelineDesc& desc)
    : mDevice(device), mType(PipelineType::Compute)
{
    assert(device != nullptr);

    const ShaderDesc& shader = desc.shader;

    const std::filesystem::path filepath = GRACE_SPIRV_DIR "/" + shader.name;

    assert(std::filesystem::exists(filepath));
    assert(shader.name.ends_with(".spv"));

    VkShaderModule shaderModule = nullptr;
    CreateShaderModule(mDevice, filepath, shaderModule);
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

    DebugReporter::Check(vkCreateComputePipelines(mDevice, nullptr, 1, &cpci, nullptr, &mPipeline));

    vkDestroyShaderModule(mDevice, shaderModule, nullptr);

    if (mPipeline != nullptr)
    {
        AssignDebugName<VkPipeline>(mDevice, mPipeline, desc.name);
    }
}

Pipeline::Pipeline(Pipeline&& other) noexcept : mDevice(other.mDevice), mPipeline(other.mPipeline), mType(other.mType)
{
    other.mDevice = nullptr;
    other.mPipeline = nullptr;
    other.mType = PipelineType::Undefined;
}

Pipeline& Pipeline::operator=(Pipeline&& other) noexcept
{
    if (mPipeline != nullptr)
    {
        vkDestroyPipeline(mDevice, mPipeline, nullptr);
    }

    mDevice = other.mDevice;
    mPipeline = other.mPipeline;
    mType = other.mType;
    other.mDevice = nullptr;
    other.mPipeline = nullptr;
    other.mType = PipelineType::Undefined;

    return *this;
}

bool Pipeline::IsNull() const
{
    return mPipeline == nullptr;
}

VkPipeline Pipeline::GetVkHandle() const
{
    return mPipeline;
}

VkPipelineBindPoint Pipeline::BindPoint() const
{
    switch (mType)
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
