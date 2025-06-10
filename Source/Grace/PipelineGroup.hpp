#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Grace
{

enum class PipelineType
{
    Compute,
    Graphics
};

struct PipelineLayoutDesc
{
    VkPipelineLayoutCreateFlags flags;
    std::vector<VkDescriptorSetLayout> setLayouts;
    std::vector<VkPushConstantRange> pushConstantRanges;
};

class PipelineLayout
{
public:
    void Create(VkDevice device, const PipelineLayoutDesc& pld);

    void Cleanup(VkDevice device);

    [[nodiscard]] bool IsNull() const;

    [[nodiscard]] VkPipelineLayout GetVkPipelineLayout() const;

private:
    VkPipelineLayout m_PipelineLayout = {};
};

/// Description used to create a Pipeline object
struct PipelineDesc
{
    /// Name used to identify the pipeline, e.g. in validation errors
    std::string name = {};
    /// Specifies type of pipeline, compute or graphics
    PipelineType type = {};
    /// Structure required to create a compute pipeline
    VkComputePipelineCreateInfo computeCreateInfo = {};
    /// Structure required to create a graphics pipeline
    VkGraphicsPipelineCreateInfo graphicsCreateInfo = {};
};

struct Pipeline
{
    Pipeline() = default;
    ~Pipeline() = default;

    void Create(VkDevice device, const PipelineDesc& desc);

    void Cleanup(VkDevice device);

    VkPipeline pipeline = {};
};

class PipelineBuilder
{
public:
    PipelineBuilder(VkDevice device);
    PipelineBuilder() = delete;
    ~PipelineBuilder();

    PipelineBuilder& BuildComputePipeline(const std::string& name, const PipelineLayout& layout);
    PipelineBuilder& BuildGraphicsPipeline(const std::string& name, const PipelineLayout& layout);

    PipelineBuilder& ClearAll();
    PipelineBuilder& ClearShaders();

    PipelineBuilder& AddShader(const std::string& shader, VkShaderStageFlagBits stage);
    PipelineBuilder& SetInputTopology(VkPrimitiveTopology topology);
    PipelineBuilder& SetPolygonMode(VkPolygonMode mode);
    PipelineBuilder& SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    PipelineBuilder& SetMultisamplingNone();
    PipelineBuilder& SetMultisampling(VkSampleCountFlagBits sampleCount);
    PipelineBuilder& SetColourAttachmentFormat(const VkFormat* pFormat);
    PipelineBuilder& SetDepthFormat(VkFormat format);
    PipelineBuilder& DisableBlending();
    PipelineBuilder& DisableDepthTest();
    PipelineBuilder& EnableDepthTest(bool depthWriteEnable, VkCompareOp op);
    PipelineBuilder& EnableBlendingAdditive();
    PipelineBuilder& EnableBlendingAlphaBlend();

public:
    PipelineDesc pipelineDesc = {};

private:
    VkDevice m_Device = {};

    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages = {};
    std::vector<VkShaderModule> m_ShaderModules = {};

    VkPipelineVertexInputStateCreateInfo m_VertexInputInfo = {};
    VkPipelineInputAssemblyStateCreateInfo m_InputAssembly = {};
    VkPipelineRasterizationStateCreateInfo m_Rasterizer = {};
    VkPipelineColorBlendAttachmentState m_ColourBlendAttachment = {};
    VkPipelineColorBlendStateCreateInfo m_ColourBlending = {};
    VkPipelineMultisampleStateCreateInfo m_Multisampling = {};
    VkPipelineDepthStencilStateCreateInfo m_DepthStencil = {};
    VkPipelineRenderingCreateInfo m_RenderInfo = {};
    VkPipelineViewportStateCreateInfo m_ViewportState = {};
    VkPipelineDynamicStateCreateInfo m_DynamicInfo = {};
    VkDynamicState m_DynamicState[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkFormat m_ColourAttachmentFormat = {};
};

void CreateComputePipeline(VkDevice device,
                           VkPipeline* pipelineOut,
                           VkPipelineLayout pipelineLayout,
                           const std::filesystem::path& path,
                           const std::string& name);

} // namespace Grace
