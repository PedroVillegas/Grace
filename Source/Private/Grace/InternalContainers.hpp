#pragma once

#include <array>
#include <cstdint>

namespace Grace
{

enum class ImageLayout : uint8_t
{
    Optimal, // Choose the most optimal layout for each usage. Performs layout transitions as appropriate for the access.
    General, // Layout accessible by all Vulkan access types on a device - no layout transitions except for presentation

    // Requires VK_KHR_shared_presentable_image to be enabled. Can only be used for shared presentable images (i.e. single-buffered swap chains).
    GeneralAndPresentation // As GENERAL, but also allows presentation engines to access it - no layout transitions
};

struct AccessInfo
{
    VkPipelineStageFlags2 stageMask;
    VkAccessFlags2 accessMask;
    VkImageLayout imageLayout;
};

static const std::array<AccessInfo, 68> AccessTypeMap = {
    { // AccessType::None
      { .stageMask = VK_PIPELINE_STAGE_2_NONE,
        .accessMask = VK_ACCESS_2_NONE,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },

      // Read Access
      // AccessType::IndirectBuffer
      { .stageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
        .accessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },
      // AccessType::IndexBuffer
      { .stageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
        .accessMask = VK_ACCESS_2_INDEX_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },
      // AccessType::VertexBuffer
      { .stageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
        .accessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },

      // AccessType::VertexShaderUniformRead
      { .stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .accessMask = VK_ACCESS_2_UNIFORM_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },
      // AccessType::VertexShaderSampledRead
      { .stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL },
      // AccessType::VertexShaderStorageRead
      { .stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },

      // AccessType::TessellationControlShaderUniformRead
      { .stageMask = VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
        .accessMask = VK_ACCESS_2_UNIFORM_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },
      // AccessType::TessellationControlShaderSampledRead
      { .stageMask = VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL },
      // AccessType::TessellationControlShaderStorageRead
      { .stageMask = VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },

      // AccessType::TessellationEvaluationShaderUniformRead
      { .stageMask = VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
        .accessMask = VK_ACCESS_2_UNIFORM_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },
      // AccessType::TessellationEvaluationShaderSampledRead
      { .stageMask = VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL },
      // AccessType::TessellationEvaluationShaderStorageRead
      { .stageMask = VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },

      // AccessType::GeometryShaderUniformRead
      { .stageMask = VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
        .accessMask = VK_ACCESS_2_UNIFORM_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },
      // AccessType::GeometryShaderSampledRead
      { .stageMask = VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL },
      // AccessType::GeometryShaderStorageRead
      { .stageMask = VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },

      // AccessType::TaskShaderUniformRead
      { .stageMask = VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
        .accessMask = VK_ACCESS_2_UNIFORM_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },
      // AccessType::TaskShaderSampledRead
      { .stageMask = VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
        .accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL },
      // AccessType::TaskShaderStorageRead
      { .stageMask = VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
        .accessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },

      // AccessType::MeshShaderUniformRead
      { .stageMask = VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT,
        .accessMask = VK_ACCESS_2_UNIFORM_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },
      // AccessType::MeshShaderSampledRead
      { .stageMask = VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT,
        .accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL },
      // AccessType::MeshShaderStorageRead
      { .stageMask = VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT,
        .accessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },

      // AccessType::FragmentShaderUniformRead
      { .stageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .accessMask = VK_ACCESS_2_UNIFORM_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },
      // AccessType::FragmentShaderSampledRead
      { .stageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL },
      // AccessType::FragmentShaderColorAttachmentRead
      { .stageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL },
      // AccessType::FragmentShaderDepthStencilAttachmentRead
      { .stageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .accessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL },
      // AccessType::FragmentShaderStorageRead
      { .stageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },

      // AccessType::ColorAttachmentRead
      { .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
      // AccessType::DepthStencilAttachmentRead
      { .stageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        .accessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL },

      // AccessType::ComputeShaderUniformRead
      { .stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .accessMask = VK_ACCESS_2_UNIFORM_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },
      // AccessType::ComputeShaderSampledRead
      { .stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL },
      // AccessType::ComputeShaderStorageRead
      { .stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },

      // AccessType::AnyShaderUniformRead
      { .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .accessMask = VK_ACCESS_2_UNIFORM_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },
      // AccessType::AnyShaderSampledRead
      { .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL },
      // AccessType::AnyShaderStorageRead
      { .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .accessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },

      // AccessType::CopyRead
      { .stageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
        .accessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
      // AccessType::ResolveRead
      { .stageMask = VK_PIPELINE_STAGE_2_RESOLVE_BIT,
        .accessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
      // AccessType::BlitRead
      { .stageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
        .accessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
      // AccessType::ClearRead
      { .stageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT,
        .accessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
      // AccessType::AnyTransferRead
      { .stageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
        .accessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },

      // AccessType::HostRead
      { .stageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
        .accessMask = VK_ACCESS_2_HOST_READ_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
      // AccessType::Present
      { .stageMask = VK_PIPELINE_STAGE_2_NONE,
        .accessMask = VK_ACCESS_2_NONE,
        .imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },

      // AccessType::RayTracingShaderAccelerationStructureRead
      { .stageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
        .accessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },
      // AccessType::AccelerationStructureBuildRead
      { .stageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        .accessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },

      // AccessType::EndOfReadAccess
      { .stageMask = VK_PIPELINE_STAGE_2_NONE,
        .accessMask = VK_ACCESS_2_NONE,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },

      // Write access
      // AccessType::VertexShaderWrite
      { .stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
      // AccessType::TessellationControlShaderWrite
      { .stageMask = VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
      // AccessType::TessellationEvaluationShaderWrite
      { .stageMask = VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
      // AccessType::GeometryShaderWrite
      { .stageMask = VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
      // AccessType::TaskShaderWrite
      { .stageMask = VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
        .accessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
      // AccessType::MeshShaderWrite
      { .stageMask = VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT,
        .accessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
      // AccessType::FragmentShaderWrite
      { .stageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
      // AccessType::ColorAttachmentWrite
      { .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
      // AccessType::DepthStencilAttachmentWrite
      { .stageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        .accessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
      // AccessType::ComputeShaderWrite
      { .stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },
      // AccessType::AnyShaderWrite
      { .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .accessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },

      // AccessType::CopyWrite
      { .stageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
        .accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
      // AccessType::ResolveWrite
      { .stageMask = VK_PIPELINE_STAGE_2_RESOLVE_BIT,
        .accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
      // AccessType::BlitWrite
      { .stageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
        .accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
      // AccessType::ClearWrite
      { .stageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT,
        .accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
      // AccessType::AnyTransferWrite
      { .stageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
        .accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },

      // AccessType::HostPreinitialized
      { .stageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
        .accessMask = VK_ACCESS_2_HOST_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED },
      // AccessType::HostWrite
      { .stageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
        .accessMask = VK_ACCESS_2_HOST_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },

      // AccessType::AccelerationStructureBuildWrite
      { .stageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        .accessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED },

      // AccessType::ColorAttachmentReadWrite
      { .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL },

      // AccessType::DepthStencilAttachmentReadWrite
      { .stageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        .accessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL },

      // AccessType::General
      { .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .accessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL },

      // AccessType::NumOfAccessTypes
      { .stageMask = VK_PIPELINE_STAGE_2_NONE,
        .accessMask = VK_ACCESS_2_NONE,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED } }
};

} // namespace Grace
