#pragma once

#include <type_traits>
#include <vulkan/vulkan_core.h>

namespace Grace
{

template <typename T>
    requires(std::is_enum_v<T> and requires(T e) { EnableBitmaskOrOp(e); })
constexpr auto operator|(const T lhs, const T rhs)
{
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

#define GRACE_ENUM_ENABLE_BITMASK_OR_OP(Enum) consteval void EnableBitmaskOrOp(Enum)

template <typename T>
    requires(std::is_enum_v<T> and requires(T e) { EnableBitmaskAndOp(e); })
constexpr auto operator&(const T lhs, const T rhs)
{
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

#define GRACE_ENUM_ENABLE_BITMASK_AND_OP(Enum) consteval void EnableBitmaskAndOp(Enum)

/// Returns `true` if `enumBitmask` has `bit` set, `false` otherwise
template <typename T>
    requires(std::is_enum_v<T>) and requires(T e) { EnableBitmaskAndOp(e); }
constexpr bool EnumBitmaskHasBitSet(const T enumBitmask, const T bit)
{
    return static_cast<bool>(enumBitmask & bit);
}

template <typename Enum1, typename Enum2>
    requires std::is_enum_v<Enum1> and std::is_enum_v<Enum2>
         and std::is_same_v<std::underlying_type_t<Enum1>, std::underlying_type_t<Enum2>>
constexpr const Enum2* EnumCastSafe(const Enum1* enum_)
{
    return reinterpret_cast<const Enum2*>(enum_);
}

/// Defines a bunch of potential resource usages
enum class AccessType : uint32_t
{
    None = 0, // No access. Useful primarily for initialization

    // Read access
    IndirectBuffer, // Read as an indirect buffer for drawing or dispatch
    IndexBuffer,    // Read as an index buffer for drawing
    VertexBuffer,   // Read as a vertex buffer for drawing

    VertexShaderUniformRead,              // Read as a uniform buffer in a vertex shader
    VertexShaderSampledRead,              // Read as a sampled image/uniform texel buffer in a vertex shader
    VertexShaderStorageRead,              // Read as any other resource in a vertex shader
    TessellationControlShaderUniformRead, // Read as a uniform buffer in a tessellation control shader
    TessellationControlShaderSampledRead, // Read as a sampled image/uniform texel buffer  in a tessellation control shader
    TessellationControlShaderStorageRead,    // Read as any other resource in a tessellation control shader
    TessellationEvaluationShaderUniformRead, // Read as a uniform buffer in a tessellation evaluation shader
    TessellationEvaluationShaderSampledRead, // Read as a sampled image/uniform texel buffer in a tessellation evaluation shader
    TessellationEvaluationShaderStorageRead,  // Read as any other resource in a tessellation evaluation shader
    GeometryShaderUniformRead,                // Read as a uniform buffer in a geometry shader
    GeometryShaderSampledRead,                // Read as a sampled image/uniform texel buffer  in a geometry shader
    GeometryShaderStorageRead,                // Read as any other resource in a geometry shader
    TaskShaderUniformRead,                    // Read as a uniform buffer in a task shader
    TaskShaderSampledRead,                    // Read as a sampled image/uniform texel buffer in a task shader
    TaskShaderStorageRead,                    // Read as any other resource in a task shader
    MeshShaderUniformRead,                    // Read as a uniform buffer in a mesh shader
    MeshShaderSampledRead,                    // Read as a sampled image/uniform texel buffer in a mesh shader
    MeshShaderStorageRead,                    // Read as any other resource in a mesh shader
    FragmentShaderUniformRead,                // Read as a uniform buffer in a fragment shader
    FragmentShaderSampledRead,                // Read as a sampled image/uniform texel buffer  in a fragment shader
    FragmentShaderColorAttachmentRead,        // Read as an input attachment in a fragment shader
    FragmentShaderDepthStencilAttachmentRead, // Read as an input attachment in a fragment shader
    FragmentShaderStorageRead,                // Read as any other resource in a fragment shader
    ColorAttachmentRead,                      // Read by standard blending/logic operations or subpass load operations
    DepthStencilAttachmentRead,               // Read by depth/stencil tests or subpass load operations
    ComputeShaderUniformRead,                 // Read as a uniform buffer in a compute shader
    ComputeShaderSampledRead,                 // Read as a sampled image/uniform texel buffer in a compute shader
    ComputeShaderStorageRead,                 // Read as any other resource in a compute shader
    AnyShaderUniformRead,
    AnyShaderSampledRead,
    AnyShaderStorageRead,

    CopyRead,    // Read as the source of a copy operation
    ResolveRead, // Read as the source of a resolve operation
    BlitRead,    // Read as the source of a blit operation
    ClearRead,   // Read as the source of a clear operation
    AnyTransferRead,
    HostRead, // Read on the host

    // Requires VK_KHR_swapchain to be enabled
    Present, // Read by the presentation engine (i.e. vkQueuePresentKHR)

    RayTracingShaderAccelerationStructureRead, // Read by a ray tracing shader as an acceleration structure
    AccelerationStructureBuildRead,            // Read as an acceleration structure during a build

    EndOfReadAccess,

    // Write access
    VertexShaderWrite,                 // Written as any resource in a vertex shader
    TessellationControlShaderWrite,    // Written as any resource in a tessellation control shader
    TessellationEvaluationShaderWrite, // Written as any resource in a tessellation evaluation shader
    GeometryShaderWrite,               // Written as any resource in a geometry shader

    TaskShaderWrite, // Written as any resource in a task shader
    MeshShaderWrite, // Written as any resource in a mesh shader

    FragmentShaderWrite,         // Written as any resource in a fragment shader
    ColorAttachmentWrite,        // Written as a color attachment during rendering, or via a subpass store op
    DepthStencilAttachmentWrite, // Written as a depth/stencil attachment during rendering, or via a subpass store op

    // Requires VK_KHR_maintenance2 to be enabled
    // DepthAttachmentWriteStencilReadOnly, // Written as a depth aspect of a depth/stencil attachment during rendering, whilst the stencil aspect is read-only
    // StencilAttachmentWriteDepthReadOnly, // Written as a stencil aspect of a depth/stencil attachment during rendering, whilst the depth aspect is read-only

    ComputeShaderWrite, // Written as any resource in a compute shader
    AnyShaderWrite,     // Written as any resource in any shader
    CopyWrite,          // Written as the destination of a copy operation
    ResolveWrite,       // Written as the destination of a resolve operation
    BlitWrite,          // Written as the destination of a blit operation
    ClearWrite,         // Written as the destination of a clear operation
    AnyTransferWrite,   // Written as the destination of any transfer operation

    HostPreinitialized, // Data pre-filled by host before device access starts
    HostWrite,          // Written on the host

    AccelerationStructureBuildWrite, // Written as an acceleration structure during a build

    ColorAttachmentReadWrite, // Read or written as a color attachment during rendering
    DepthStencilAttachmentReadWrite, // Read or written as a depth or stencil attachment during rendering

    // General access
    General, // Covers any access - useful for debug, generally avoid for performance reasons

    NumOfAccessTypes,
};

/// Specifies how a given resource should be accessed from within a shader
enum class DescriptorType : uint32_t
{
    /// Specifies a descriptor for a Sampler object
    Sampler = VK_DESCRIPTOR_TYPE_SAMPLER,
    /// Specifies a descriptor for an ImageView and a Sampler object pair as a read-only sampled image using
    /// the provided sampler
    CombinedImageSampler = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    /// Specifies a descriptor for an ImageView object as a read-only sampled image
    SampledImage = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    /// Specifies a descriptor for a ImageView object as a read/write storage image
    StorageImage = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    /// Specifies a descriptor for a Buffer object as a formatted read-only buffer
    TexelBuffer = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
    /// Specifies a descriptor for a Buffer object as a formatted read/write buffer
    StorageTexelBuffer = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
    /// Specifies a descriptor for a Buffer object as a read-only uniform (constant) buffer
    UniformBuffer = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    /// Specifies a descriptor for a Buffer object as a read/write buffer
    StorageBuffer = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    /// Specifies a descriptor for a Buffer object as a read-only uniform (constant) buffer with a dynamic offset
    UniformBufferDynamic = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
    /// Specifies a descriptor for a Buffer object as a read/write buffer with a dynamic offset
    StorageBufferDynamic = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
};

/// Specifies how consecutive vertices are organised into primitives
enum class Topology : uint32_t
{
    /// Specifies a series of separate point primitives
    PointList = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
    /// Specifies a series of separate line primitives
    LineList = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
    /// Specifies a series of connected line primitives with consecutive lines sharing a vertex
    LineStrip = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
    /// Specifies a series of separate triangle primitives
    TriangleList = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    /// Specifies a series of connected triangle primitives with consecutive triangles sharing an edge
    TriangleStrip = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    /// Specifies a series of connected triangle primitives with all triangles sharing a common vertex
    TriangleFan = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
    /// Specifies a series of separate line primitives with adjacency
    LineListWithAdjacency = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
    /// Specifies a series of connected line primitives with adjacency, with consecutive primitives sharing three
    /// vertices
    LineStripWithAdjacency = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
    /// Specifies a series of separate triangle primitives with adjacency
    TriangleListWithAdjacency = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
    /// Specifies connected triangle primitives with adjacency, with consecutive triangles sharing an edge
    TriangleStripWithAdjacency = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
    /// Specifies separate patch primitives
    PatchList = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
};

/// Specifies the method of rasterization for polygons
enum class PolygonMode : uint32_t
{
    /// Specifies the polygons are rendered filled in, obeying a set of rasterization rules
    Fill = VK_POLYGON_MODE_FILL,
    /// Specifies the polygons are drawn as line segments
    Line = VK_POLYGON_MODE_LINE,
    /// Specifies the polygons are drawn as points
    Point = VK_POLYGON_MODE_POINT
};

/// Specifies shader stage
enum class ShaderStage : uint32_t
{
    /// Specifies the vertex stage
    Vertex = VK_SHADER_STAGE_VERTEX_BIT,
    /// Specifies the tessellation control stage
    TessellationControl = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
    /// Specifies the tessellation evaluation stage
    TessellationEvaluation = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
    /// Specifies the geometry stage
    Geometry = VK_SHADER_STAGE_GEOMETRY_BIT,
    /// Specifies the fragment stage
    Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
    /// Specifies the compute stage
    Compute = VK_SHADER_STAGE_COMPUTE_BIT,
    /// Combination of bits used as shorthand to specify all graphics stages
    AllGraphics = VK_SHADER_STAGE_ALL_GRAPHICS,
    /// Combination of bits used as shorthand to specify all shader stages supported by the device,
    /// including all additional stages which are introduced by extensions
    All = VK_SHADER_STAGE_ALL
};
GRACE_ENUM_ENABLE_BITMASK_OR_OP(ShaderStage);
GRACE_ENUM_ENABLE_BITMASK_AND_OP(ShaderStage);

// Specifies how triangles are culled once the orientation is determined
enum class CullMode : uint32_t
{
    /// Specifies that no triangles are discarded
    None,
    /// Specifies that front-facing triangles are discarded
    Front = VK_CULL_MODE_FRONT_BIT,
    /// Specifies that back-facing triangles are discarded
    Back = VK_CULL_MODE_BACK_BIT,
    /// Specifies that all triangles are discarded
    FrontAndBack = VK_CULL_MODE_FRONT_AND_BACK,
};

/// Specifies the winding direction corresponding to the front-face of a polygon
enum class FrontFace : uint32_t
{
    CounterClockwise = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    Clockwise = VK_FRONT_FACE_CLOCKWISE,
};

/// Comparison operators used for:
/// - Depth Compare Operation for a sampler
/// - Stencil Comparison during stencil test
/// - Depth Comparison during depth test
enum class CompareOp : uint32_t
{
    /// Specifies that the comparison always evaluates false
    Never = VK_COMPARE_OP_NEVER,
    /// Specifies that the comparison evaluates reference < test
    Less = VK_COMPARE_OP_LESS,
    /// Specifies that the comparison evaluates reference = test
    Equal = VK_COMPARE_OP_EQUAL,
    /// Specifies that the comparison evaluates reference ≤ test
    LessOrEqual = VK_COMPARE_OP_LESS_OR_EQUAL,
    /// Specifies that the comparison evaluates reference > test
    Greater = VK_COMPARE_OP_GREATER,
    /// Specifies that the comparison evaluates reference ≠ test
    NotEqual = VK_COMPARE_OP_NOT_EQUAL,
    /// Specifies that the comparison evaluates reference ≥ test
    GreaterOrEqual = VK_COMPARE_OP_GREATER_OR_EQUAL,
    /// Specifies that the comparison always evaluates true
    Always = VK_COMPARE_OP_ALWAYS
};

/// Specifies what happens to the stored stencil value if this or certain subsequent tests fail or pass
enum class StencilOp : uint32_t
{
    /// Keeps the current value
    Keep = VK_STENCIL_OP_KEEP,
    /// Sets the value to 0
    Zero = VK_STENCIL_OP_ZERO,
    /// Sets the value to reference
    Replace = VK_STENCIL_OP_REPLACE,
    /// Increments the current value and clamps to the maximum representable unsigned value
    IncrementAndClamp = VK_STENCIL_OP_INCREMENT_AND_CLAMP,
    /// Decrements the current value and clamps to 0
    DecrementAndClamp = VK_STENCIL_OP_DECREMENT_AND_CLAMP,
    /// Bitwise-inverts the current value
    Invert = VK_STENCIL_OP_INVERT,
    /// Increments the current value and wraps to 0 when the maximum value would have been exceeded
    IncrementAndWrap = VK_STENCIL_OP_INCREMENT_AND_WRAP,
    /// Decrements the current value and wraps to the maximum possible value when the value would go below 0
    DecrementAndWrap = VK_STENCIL_OP_DECREMENT_AND_WRAP
};

/// Specifies the logical operation applied between the fragment’s color values and the existing
/// value in the framebuffer attachment prior to updating the framebuffer attachment
enum class LogicOp : uint32_t
{
    Clear = VK_LOGIC_OP_CLEAR,
    And = VK_LOGIC_OP_AND,
    AndReverse = VK_LOGIC_OP_AND_REVERSE,
    Copy = VK_LOGIC_OP_COPY,
    AndInverted = VK_LOGIC_OP_AND_INVERTED,
    NoOp = VK_LOGIC_OP_NO_OP,
    Xor = VK_LOGIC_OP_XOR,
    Or = VK_LOGIC_OP_OR,
    Nor = VK_LOGIC_OP_NOR,
    Equivalent = VK_LOGIC_OP_EQUIVALENT,
    Invert = VK_LOGIC_OP_INVERT,
    OrReverse = VK_LOGIC_OP_OR_REVERSE,
    CopyInverted = VK_LOGIC_OP_COPY_INVERTED,
    OrInverted = VK_LOGIC_OP_OR_INVERTED,
    Nand = VK_LOGIC_OP_NAND,
    Set = VK_LOGIC_OP_SET
};

/// Specifies the blending operation applied between source and destination components and blend factors
enum class BlendOp : uint32_t
{
    Add = VK_BLEND_OP_ADD,
    Subtract = VK_BLEND_OP_SUBTRACT,
    ReverseSubtract = VK_BLEND_OP_REVERSE_SUBTRACT,
    Min = VK_BLEND_OP_MIN,
    Max = VK_BLEND_OP_MAX
};

/// Specifies the source and destination color and alpha blending factors
enum class BlendFactor : uint32_t
{
    Zero = VK_BLEND_FACTOR_ZERO,
    One = VK_BLEND_FACTOR_ONE,
    SrcColor = VK_BLEND_FACTOR_SRC_COLOR,
    OneMinusSrcColor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    DstColor = VK_BLEND_FACTOR_DST_COLOR,
    OneMinusDstColor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    SrcAlpha = VK_BLEND_FACTOR_SRC_ALPHA,
    OneMinusSrcAlpha = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    DstAlpha = VK_BLEND_FACTOR_DST_ALPHA,
    OneMinusDstAlpha = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    ConstantColor = VK_BLEND_FACTOR_CONSTANT_COLOR,
    OneMinusConstantColor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
    ConstantAlpha = VK_BLEND_FACTOR_CONSTANT_ALPHA,
    OneMinusConstantAlpha = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
    SrcAlphaSaturate = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
    Src1Color = VK_BLEND_FACTOR_SRC1_COLOR,
    OneMinusSrc1Color = VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
    Src1Alpha = VK_BLEND_FACTOR_SRC1_ALPHA,
    OneMinusSrc1Alpha = VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
};

/// Sample counts that can be used for image storage operations
enum class MultisampleLevel : uint32_t
{
    /// Specifies an image with one sample per pixel
    x1 = VK_SAMPLE_COUNT_1_BIT,
    /// Specifies an image with 2 samples per pixel
    x2 = VK_SAMPLE_COUNT_2_BIT,
    /// Specifies an image with 4 samples per pixel
    x4 = VK_SAMPLE_COUNT_4_BIT,
    /// Specifies an image with 8 samples per pixel
    x8 = VK_SAMPLE_COUNT_8_BIT,
    /// Specifies an image with 16 samples per pixel
    x16 = VK_SAMPLE_COUNT_16_BIT,
    /// Specifies an image with 32 samples per pixel
    x32 = VK_SAMPLE_COUNT_32_BIT,
    /// Specifies an image with 64 samples per pixel
    x64 = VK_SAMPLE_COUNT_64_BIT,
};

/// Specifies how multisample values in a multisample attachment are combined
enum class ResolveMode : uint32_t
{
    /// Specifies that no resolve operation is done
    None = VK_RESOLVE_MODE_NONE,
    /// Specifies that result of the resolve operation is equal to the value of sample 0
    SampleZero = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT,
    /// Specifies that result of the resolve operation is the average of the sample values
    Average = VK_RESOLVE_MODE_AVERAGE_BIT,
    /// Specifies that result of the resolve operation is the minimum of the sample values
    Min = VK_RESOLVE_MODE_MIN_BIT,
    /// Specifies that result of the resolve operation is the maximum of the sample values
    Max = VK_RESOLVE_MODE_MAX_BIT
};

/// Specifies the component values placed in each component of the output vector
enum class ComponentSwizzle : uint32_t
{
    /// Specifies that the component is set to the identity swizzle
    Identity = VK_COMPONENT_SWIZZLE_IDENTITY,
    /// Specifies that the component is set to zero
    Zero = VK_COMPONENT_SWIZZLE_ZERO,
    /// Specifies that the component is set to either 1 or 1.0
    One = VK_COMPONENT_SWIZZLE_ONE,
    /// Specifies that the component is set to the value of the R component of the image
    R = VK_COMPONENT_SWIZZLE_R,
    /// Specifies that the component is set to the value of the G component of the image
    G = VK_COMPONENT_SWIZZLE_G,
    /// Specifies that the component is set to the value of the B component of the image
    B = VK_COMPONENT_SWIZZLE_B,
    /// Specifies that the component is set to the value of the A component of the image
    A = VK_COMPONENT_SWIZZLE_A
};

/// Specifies whether the final color values R, G, B and A are written to the framebuffer attachment
enum class ColorComponent : uint32_t
{
    /// Specifies that the R value is written to the color attachment for the appropriate sample.
    /// Otherwise, the value in memory is unmodified
    Red = VK_COLOR_COMPONENT_R_BIT,
    /// Specifies that the G value is written to the color attachment for the appropriate sample.
    /// Otherwise, the value in memory is unmodified
    Green = VK_COLOR_COMPONENT_G_BIT,
    /// Specifies that the B value is written to the color attachment for the appropriate sample.
    /// Otherwise, the value in memory is unmodified
    Blue = VK_COLOR_COMPONENT_B_BIT,
    /// Specifies that the A value is written to the color attachment for the appropriate sample.
    /// Otherwise, the value in memory is unmodified
    Alpha = VK_COLOR_COMPONENT_A_BIT
};
GRACE_ENUM_ENABLE_BITMASK_OR_OP(ColorComponent);
GRACE_ENUM_ENABLE_BITMASK_AND_OP(ColorComponent);

enum class ColorBlendMode : uint32_t
{
    NoBlend = 0,
    Additive = 1,
    AlphaBlend = 2,
};

enum class DepthStencilUsage : uint32_t
{
    None = 0,
    /// Specifies only depth test enabled
    DepthTestNoWrite = 1,
    /// Specifies only depth test and write enabled
    DepthOnly = 2,
    /// Specifies only stencil test enabled
    StencilOnly = 3,
    /// Specifies depth test enabled, depth write enabled and stencil test enabled
    All = 4,
    // TODO: add depth bounds test
};

/// Specifies which pieces of pipeline state will use the values from
/// dynamic state commands rather than from pipeline state creation information
enum class DynamicState : uint32_t
{
    /// Specifies that the line width state must be set dynamically with CmdSetLineWidth()
    LineWidth = VK_DYNAMIC_STATE_LINE_WIDTH,
    /// Specifies that the depth bias state must be set dynamically with CmdSetDepthBias()
    DepthBias = VK_DYNAMIC_STATE_DEPTH_BIAS,
    /// Specifies that the depth bias state must be set dynamically with CmdSetBlendConstants()
    BlendConstants = VK_DYNAMIC_STATE_BLEND_CONSTANTS,
    /// Specifies that the depth bias state must be set dynamically with CmdSetDepthBounds()
    DepthBounds = VK_DYNAMIC_STATE_DEPTH_BOUNDS,
    /// Specifies that the stencil compare mask state must be set dynamically with CmdSetStencilCompareMask()
    StencilCompareMask = VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
    /// Specifies that the stencil write mask state must be set dynamically with CmdSetStencilWriteMask()
    StencilWriteMask = VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
    /// Specifies that the stencil reference state must be set dynamically with CmdSetStencilReference()
    StencilRef = VK_DYNAMIC_STATE_STENCIL_REFERENCE,
    /// Specifies that the cull mode state must be set dynamically with CmdSetCullMode()
    CullMode = VK_DYNAMIC_STATE_CULL_MODE,
    /// Specifies that the front face state must be set dynamically with CmdSetFrontFace()
    FrontFace = VK_DYNAMIC_STATE_FRONT_FACE,
    /// Specifies that the primitive topology state must be set dynamically with CmdSetPrimitiveTopology()
    PrimitiveTopology = VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY,
    /// Specifies that the vertex input binding stride state must be set dynamically with CmdBindVertexBuffers2()
    VertexInputBindingStride = VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE,
    /// Specifies that the depth test enable state must be set dynamically with CmdSetDepthTestEnable()
    DepthTestEnable = VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
    /// Specifies that the depth write enable state must be set dynamically with CmdSetDepthWriteEnable()
    DepthWriteEnable = VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
    /// Specifies that the depth compare op state must be set dynamically with CmdSetDepthCompareOp()
    DepthCompOp = VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
    /// Specifies that the depth bounds test enable state must be set dynamically with CmdSetDepthBoundsTestEnable()
    DepthBoundsTestEnable = VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE,
    /// Specifies that the stencil test enable state must be set dynamically with CmdSetStencilTestEnable()
    StencilTestEnable = VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE,
    /// Specifies that the stencil op state must be set dynamically with CmdSetStencilOp()
    StencilOp = VK_DYNAMIC_STATE_STENCIL_OP,
    /// Specifies that the rasterizer discard enable state must be set dynamically with CmdSetRasterizerDiscardEnable()
    RasterizerDiscardEnable = VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE,
    /// Specifies that the depth bias enable state must be set dynamically with CmdSetDepthBiasEnable()
    DepthBiasEnable = VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE,
    /// Specifies that the primitive restart enable state must be set dynamically with CmdSetPrimitiveRestartEnable()
    PrimitiveRestartEnable = VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE
};

/// Specifies the intended usage of image
enum class ImageUsage : uint32_t
{
    /// Specifies the image can be used as the source image of copy, resolve and blit commands
    TransferSrc = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    /// Specifies the image can be used as the destination image of copy, resolve and blit commands
    TransferDst = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    /// Specifies the image can be used as a sampled image or in a combined image sampler
    SampledImage = VK_IMAGE_USAGE_SAMPLED_BIT,
    /// Specifies the image can be used as a storage image
    StorageImage = VK_IMAGE_USAGE_STORAGE_BIT,
    /// Specifies the image can be used as a color or resolve attachment in a framebuffer
    ColorAttachment = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    /// Specifies the image can be used as a depth/stencil or depth/stencil resolve attachment in a framebuffer
    DepthStencilAttachment = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    /// Specifies the image can be read from a shader as an input attachment
    /// and be used as an input attachment in a framebuffer
    InputAttachment = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
};
GRACE_ENUM_ENABLE_BITMASK_OR_OP(ImageUsage);
GRACE_ENUM_ENABLE_BITMASK_AND_OP(ImageUsage);

/// Specifies the aspect of an image for purposes such as identifying a subresource
enum class ImageAspect : uint32_t
{
    /// Specifies no image aspect, or the image aspect is not applicable
    None = VK_IMAGE_ASPECT_NONE,
    /// Specifies the color aspect
    Color = VK_IMAGE_ASPECT_COLOR_BIT,
    /// Specifies the depth aspect
    Depth = VK_IMAGE_ASPECT_DEPTH_BIT,
    /// Specifies the stencil aspect
    Stencil = VK_IMAGE_ASPECT_STENCIL_BIT,
    /// Specifies the metadata aspect used for sparse resource operations
    Metadata = VK_IMAGE_ASPECT_METADATA_BIT
};
GRACE_ENUM_ENABLE_BITMASK_OR_OP(ImageAspect);
GRACE_ENUM_ENABLE_BITMASK_AND_OP(ImageAspect);

/// Specifies the intended buffer usage
enum class BufferUsage : uint32_t
{
    // Buffer can be used as the source of a transfer command
    TransferSrc = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    // Buffer can be used as the destination of a transfer command
    TransferDst = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    // Buffer can be used as a uniform texel buffer in a shader
    UniformTexel = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
    // Buffer can be used as a storage texel buffer in a shader
    StorageTexel = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
    // Buffer can be used as a uniform buffer in a shader
    UniformBuffer = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    // Buffer can be used as a storage buffer in a shader
    StorageBuffer = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    // Buffer can be used as an index buffer
    IndexBuffer = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    // Buffer can be used as a vertex buffer
    VertexBuffer = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    // Buffer can be used as an indirect buffer
    IndirectBuffer = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
    // Buffer can be used to retrieve a buffer device address via `Grace::Buffer::GetBDA`
    // and use that address to access the buffer’s memory from a shader
    DeviceAddress = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
};
GRACE_ENUM_ENABLE_BITMASK_OR_OP(BufferUsage);
GRACE_ENUM_ENABLE_BITMASK_AND_OP(BufferUsage);

/// Specifies the load operation applied to the contents of an attachment at the start of a render pass
enum class AttachmentLoadOp : uint32_t
{
    /// Specifies that the attachment will load the contents of the assigned image view
    Load = VK_ATTACHMENT_LOAD_OP_LOAD,
    /// Specifies that the attachment will be cleared to a specified value
    Clear = VK_ATTACHMENT_LOAD_OP_CLEAR,
    /// Specifies that the previous contents within the area need not be preserved and will be undefined
    DontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE
};

/// Specifies how the contents of the attachment are treated
enum class AttachmentStoreOp : uint32_t
{
    /// Specifies the contents generated during the render pass and within the render area are written to memory
    Store = VK_ATTACHMENT_STORE_OP_STORE,
    /// Specifies the contents within the render area are not needed after rendering, and may be discarded
    DontCare = VK_ATTACHMENT_STORE_OP_DONT_CARE
};

/// Specifies the size/type of indices
enum class IndexType : uint32_t
{
    /// Specifies that indices are 16-bit unsigned integer values
    UInt16 = VK_INDEX_TYPE_UINT16,
    /// Specifies that indices are 32-bit unsigned integer values
    UInt32 = VK_INDEX_TYPE_UINT32,
};

/// Specifies the filter used for texture lookups
enum class Filter : uint32_t
{
    /// Returns the value of the nearest texel
    Nearest = VK_FILTER_NEAREST,
    /// Returns a linear interpolation of the nearest 8 (for 3D), 4 (for 2D or Cube), or 2 (for 1D) texel values
    Linear = VK_FILTER_LINEAR,
};

/// Specifies the mipmap mode used for texture lookups
enum class SamplerMipmapMode : uint32_t
{
    /// Returns the value of a single mipmap level
    Nearest = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    /// Returns a linear interpolation of the sampled values from the two nearest mipmap levels
    Linear = VK_SAMPLER_MIPMAP_MODE_LINEAR,
};

/// Specifies the wrapping operation used when sampling with texture coordinates outside an image
enum class SamplerAddressMode : uint32_t
{
    /// Samples as if the images are tiled side-by-side
    Repeat = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    /// Samples as if the images are tiled side-by-side and every other tile is flipped along its corresponding axis
    MirroredRepeat = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    /// Samples the nearest edge of the image
    ClampToEdge = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    /// Samples the specified border color
    ClampToBorder = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
};

/// Specifies the border color used for texture lookups
enum class BorderColor : uint32_t
{
    /// Specifies a transparent, floating-point format, black color
    FloatTransparentBlack = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
    /// Specifies a transparent, integer format, black color
    IntTransparentBlack = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,
    /// Specifies an opaque, floating-point format, black color
    FloatOpaqueBlack = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
    /// Specifies an opaque, integer format, black color
    IntOpaqueBlack = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    /// Specifies an opaque, floating-point format, white color
    FloatOpaqueWhite = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    /// Specifies an opaque, integer format, white color
    IntOpaqueWhite = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
};

enum class QueueFamily : uint32_t
{
    /// Queue supporting transfer operations
    Transfer,
    /// Queue supporting transfer and compute pipeline operations
    Compute,
    /// Queue supporting transfer and graphics pipeline operations
    Graphics,
    /// Queue supporting present operations
    Present,
    /// Queue is undefined
    Undefined
};

/// Specifies the presentation mode for a surface
enum class PresentMode : uint32_t
{
    /// Presented images appear on the screen immediately, without waiting for the next v-blank interval.
    /// This mode may cause visible tearing
    Immediate = VK_PRESENT_MODE_IMMEDIATE_KHR,
    /// Presented images queue up for being displayed on the screen. During each v-blank interval, the most
    /// recent presented image will be displayed.
    Mailbox = VK_PRESENT_MODE_MAILBOX_KHR,
    /// Presented images queue up for being displayed on the screen. During each v-blank interval, the least
    /// recent presented image will be displayed. This mode may cause AcquireNextImage to wait for an
    /// image to become available, effectively tying the rate of presentation to the screen's v-blank interval.
    /// This is the only mode that is always supported.
    FIFO = VK_PRESENT_MODE_FIFO_KHR,
    /// Similar to FIFO, except if the application has not presented an image in time for the next
    /// v-blank interval, the next time an image gets presented, it will be displayed on the screen
    /// immediately. This should help smooth out the framerate, but it may also cause visible tearing in those
    /// situations.
    RelaxedFIFO = VK_PRESENT_MODE_FIFO_RELAXED_KHR,
};

enum class PipelineBindPoint : uint32_t
{
    Compute = VK_PIPELINE_BIND_POINT_COMPUTE,
    Graphics = VK_PIPELINE_BIND_POINT_GRAPHICS,
    Raytracing = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
};

/// Specifies the stage of execution in the pipeline
enum class PipelineStage : uint32_t
{
    /// Specifies no stages of execution
    None = VK_PIPELINE_STAGE_2_NONE,
    TopOfPipe = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
    /// Specifies the stage where VkDrawIndirect / VkDispatchIndirect / VkTraceRaysIndirect data structures are consumed
    DrawIndirect = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
    /// Specifies the stage where vertex and index buffers are consumed
    VertexInput = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
    /// Specifies the vertex shader stage
    VertexShader = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
    /// Specifies the tessellation control shader stage
    ControlShader = VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
    /// Specifies the tessellation evaluation shader stage
    EvaluationShader = VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
    /// Specifies the geometry shader stage
    GeometryShader = VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
    /// Specifies the fragment shader stage
    FragmentShader = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
    /// Specifies the stage where early fragment tests (depth/stencil tests before fragment shading) are performed
    EarlyFragmentTests = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
    /// Specifies the stage where late fragment tests (depth/stencil tests after fragment shading) are performed
    LateFragmentTests = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
    /// Specifies the stage after blending where the final color values are output from the pipeline
    ColorAttachmentOutput = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    /// Specifies the execution of a compute shader
    ComputeShader = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
    /// Specifies copy, blit, resolve and clear commands
    StageTransfer = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
    BottomOfPipe = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
    /// Specifies all command stages
    AllCommands = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    /// Specifies all graphics pipeline stages
    AllGraphics = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
    /// Specifies all transfer stages
    AllTransfer = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
};
GRACE_ENUM_ENABLE_BITMASK_OR_OP(PipelineStage);
GRACE_ENUM_ENABLE_BITMASK_AND_OP(PipelineStage);

/// Specifies the format that data can be stored in inside buffers and images
enum class Format : int
{
    Undefined = VK_FORMAT_UNDEFINED,

    // 8-bit color formats
    RG4_UNormPack8 = VK_FORMAT_R4G4_UNORM_PACK8,
    R8_UNorm = VK_FORMAT_R8_UNORM,
    R8_SNorm = VK_FORMAT_R8_SNORM,
    R8_UScaled = VK_FORMAT_R8_USCALED,
    R8_SScaled = VK_FORMAT_R8_SSCALED,
    R8_UInt = VK_FORMAT_R8_UINT,
    R8_SInt = VK_FORMAT_R8_SINT,
    R8_SRGB = VK_FORMAT_R8_SRGB,

    // 16-bit color formats
    RGBA4_UNormPack16 = VK_FORMAT_R4G4B4A4_UNORM_PACK16,
    BGRA4_UNormPack16 = VK_FORMAT_B4G4R4A4_UNORM_PACK16,
    R5G6B5_UNormPack16 = VK_FORMAT_R5G6B5_UNORM_PACK16,
    B5G6R5_UNormPack16 = VK_FORMAT_B5G6R5_UNORM_PACK16,
    RGB5A1_UNormPack16 = VK_FORMAT_R5G5B5A1_UNORM_PACK16,
    BGR5A1_UNormPack16 = VK_FORMAT_B5G5R5A1_UNORM_PACK16,
    A1RGB5_UNormPack16 = VK_FORMAT_A1R5G5B5_UNORM_PACK16,
    RG8_UNorm = VK_FORMAT_R8G8_UNORM,
    RG8_SNorm = VK_FORMAT_R8G8_SNORM,
    RG8_UScaled = VK_FORMAT_R8G8_USCALED,
    RG8_SScaled = VK_FORMAT_R8G8_SSCALED,
    RG8_UInt = VK_FORMAT_R8G8_UINT,
    RG8_SInt = VK_FORMAT_R8G8_SINT,
    RG8_SRGB = VK_FORMAT_R8G8_SRGB,
    R16_UNorm = VK_FORMAT_R16_UNORM,
    R16_SNorm = VK_FORMAT_R16_SNORM,
    R16_UScaled = VK_FORMAT_R16_USCALED,
    R16_SScaled = VK_FORMAT_R16_SSCALED,
    R16_UInt = VK_FORMAT_R16_UINT,
    R16_SInt = VK_FORMAT_R16_SINT,
    R16_SFloat = VK_FORMAT_R16_SFLOAT,

    // 24-bit color formats
    RGB8_UNorm = VK_FORMAT_R8G8B8_UNORM,
    RGB8_SNorm = VK_FORMAT_R8G8B8_SNORM,
    RGB8_UScaled = VK_FORMAT_R8G8B8_USCALED,
    RGB8_SScaled = VK_FORMAT_R8G8B8_SSCALED,
    RGB8_UInt = VK_FORMAT_R8G8B8_UINT,
    RGB8_SInt = VK_FORMAT_R8G8B8_SINT,
    RGB8_SRGB = VK_FORMAT_R8G8B8_SRGB,
    BGR8_UNorm = VK_FORMAT_B8G8R8_UNORM,
    BGR8_SNorm = VK_FORMAT_B8G8R8_SNORM,
    BGR8_UScaled = VK_FORMAT_B8G8R8_USCALED,
    BGR8_SScaled = VK_FORMAT_B8G8R8_SSCALED,
    BGR8_UInt = VK_FORMAT_B8G8R8_UINT,
    BGR8_SInt = VK_FORMAT_B8G8R8_SINT,
    BGR8_SRGB = VK_FORMAT_B8G8R8_SRGB,

    // 32-bit color formats
    RGBA8_UNorm = VK_FORMAT_R8G8B8A8_UNORM,
    RGBA8_SNorm = VK_FORMAT_R8G8B8A8_SNORM,
    RGBA8_UScaled = VK_FORMAT_R8G8B8A8_USCALED,
    RGBA8_SScaled = VK_FORMAT_R8G8B8A8_SSCALED,
    RGBA8_UInt = VK_FORMAT_R8G8B8A8_UINT,
    RGBA8_SInt = VK_FORMAT_R8G8B8A8_SINT,
    RGBA8_SRGB = VK_FORMAT_R8G8B8A8_SRGB,
    BGRA8_UNorm = VK_FORMAT_B8G8R8A8_UNORM,
    BGRA8_SNorm = VK_FORMAT_B8G8R8A8_SNORM,
    BGRA8_UScaled = VK_FORMAT_B8G8R8A8_USCALED,
    BGRA8_SScaled = VK_FORMAT_B8G8R8A8_SSCALED,
    BGRA8_UInt = VK_FORMAT_B8G8R8A8_UINT,
    BGRA8_SInt = VK_FORMAT_B8G8R8A8_SINT,
    BGRA8_SRGB = VK_FORMAT_B8G8R8A8_SRGB,
    ABGR8_UNormPack32 = VK_FORMAT_A8B8G8R8_UNORM_PACK32,
    ABGR8_SNormPack32 = VK_FORMAT_A8B8G8R8_SNORM_PACK32,
    ABGR8_UScaledPack32 = VK_FORMAT_A8B8G8R8_USCALED_PACK32,
    ABGR8_SScaledPack32 = VK_FORMAT_A8B8G8R8_SSCALED_PACK32,
    ABGR8_UIntPack32 = VK_FORMAT_A8B8G8R8_UINT_PACK32,
    ABGR8_SIntPack32 = VK_FORMAT_A8B8G8R8_SINT_PACK32,
    ABGR8_SRGBPack32 = VK_FORMAT_A8B8G8R8_SRGB_PACK32,
    A2RGB10_UNormPack32 = VK_FORMAT_A2R10G10B10_UNORM_PACK32,
    A2RGB10_SNormPack32 = VK_FORMAT_A2R10G10B10_SNORM_PACK32,
    A2RGB10_UScaledPack32 = VK_FORMAT_A2R10G10B10_USCALED_PACK32,
    A2RGB10_SScaledPack32 = VK_FORMAT_A2R10G10B10_SSCALED_PACK32,
    A2RGB10_UIntPack32 = VK_FORMAT_A2R10G10B10_UINT_PACK32,
    A2RGB10_SIntPack32 = VK_FORMAT_A2R10G10B10_SINT_PACK32,
    A2BGR10_UNormPack32 = VK_FORMAT_A2B10G10R10_UNORM_PACK32,
    A2BGR10_SNormPack32 = VK_FORMAT_A2B10G10R10_SNORM_PACK32,
    A2BGR10_UScaledPack32 = VK_FORMAT_A2B10G10R10_USCALED_PACK32,
    A2BGR10_SScaledPack32 = VK_FORMAT_A2B10G10R10_SSCALED_PACK32,
    A2BGR10_UIntPack32 = VK_FORMAT_A2B10G10R10_UINT_PACK32,
    A2BGR10_SIntPack32 = VK_FORMAT_A2B10G10R10_SINT_PACK32,
    RG16_UNorm = VK_FORMAT_R16G16_UNORM,
    RG16_SNorm = VK_FORMAT_R16G16_SNORM,
    RG16_UScaled = VK_FORMAT_R16G16_USCALED,
    RG16_SScaled = VK_FORMAT_R16G16_SSCALED,
    RG16_UInt = VK_FORMAT_R16G16_UINT,
    RG16_SInt = VK_FORMAT_R16G16_SINT,
    RG16_SFloat = VK_FORMAT_R16G16_SFLOAT,
    R32_UInt = VK_FORMAT_R32_UINT,
    R32_SInt = VK_FORMAT_R32_SINT,
    R32_SFloat = VK_FORMAT_R32_SFLOAT,
    B10GR11_UFloatPack32 = VK_FORMAT_B10G11R11_UFLOAT_PACK32,
    E5BGR9_UFloatPack32 = VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,

    // 48-bit color formats
    RGB16_UNorm = VK_FORMAT_R16G16B16_UNORM,
    RGB16_SNorm = VK_FORMAT_R16G16B16_SNORM,
    RGB16_UScaled = VK_FORMAT_R16G16B16_USCALED,
    RGB16_SScaled = VK_FORMAT_R16G16B16_SSCALED,
    RGB16_UInt = VK_FORMAT_R16G16B16_UINT,
    RGB16_SInt = VK_FORMAT_R16G16B16_SINT,
    RGB16_SFloat = VK_FORMAT_R16G16B16_SFLOAT,

    // 64-bit color formats
    RGBA16_UNorm = VK_FORMAT_R16G16B16A16_UNORM,
    RGBA16_SNorm = VK_FORMAT_R16G16B16A16_SNORM,
    RGBA16_UScaled = VK_FORMAT_R16G16B16A16_USCALED,
    RGBA16_SScaled = VK_FORMAT_R16G16B16A16_SSCALED,
    RGBA16_UImt = VK_FORMAT_R16G16B16A16_UINT,
    RGBA16_SImt = VK_FORMAT_R16G16B16A16_SINT,
    RGBA16_SFloat = VK_FORMAT_R16G16B16A16_SFLOAT,
    RG32_UInt = VK_FORMAT_R32G32_UINT,
    RG32_SInt = VK_FORMAT_R32G32_SINT,
    RG32_SFloat = VK_FORMAT_R32G32_SFLOAT,
    R64_UInt = VK_FORMAT_R64_UINT,
    R64_SInt = VK_FORMAT_R64_SINT,
    R64_SFloat = VK_FORMAT_R64_SFLOAT,

    // 96-bit color formats
    RGB32_UInt = VK_FORMAT_R32G32B32_UINT,
    RGB32_SInt = VK_FORMAT_R32G32B32_SINT,
    RGB32_SFloat = VK_FORMAT_R32G32B32_SFLOAT,

    // 128-bit color formats
    RGBA32_UInt = VK_FORMAT_R32G32B32A32_UINT,
    RGBA32_SInt = VK_FORMAT_R32G32B32A32_SINT,
    RGBA32_SFloat = VK_FORMAT_R32G32B32A32_SFLOAT,
    RG64_UInt = VK_FORMAT_R64G64_UINT,
    RG64_SInt = VK_FORMAT_R64G64_SINT,
    RG64_SFloat = VK_FORMAT_R64G64_SFLOAT,

    // 192-bit color formats
    RGB64_UInt = VK_FORMAT_R64G64B64_UINT,
    RGB64_SInt = VK_FORMAT_R64G64B64_SINT,
    RGB64_SFloat = VK_FORMAT_R64G64B64_SFLOAT,

    // 256-bit color formats
    RGBA64_UInt = VK_FORMAT_R64G64B64A64_UINT,
    RGBA64_SInt = VK_FORMAT_R64G64B64A64_SINT,
    RGBA64_SFloat = VK_FORMAT_R64G64B64A64_SFLOAT,

    // Compressed color formats
    COMP_BC1_RGB_UNormBlock = VK_FORMAT_BC1_RGB_UNORM_BLOCK,
    COMP_BC1_RGB_SRGBBlock = VK_FORMAT_BC1_RGB_SRGB_BLOCK,
    COMP_BC1_RGBA_UNormBlock = VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
    BC1_RGBA_SRGBBlock = VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
    BC2_UNormBlock = VK_FORMAT_BC2_UNORM_BLOCK,
    BC2_SRGBBlock = VK_FORMAT_BC2_SRGB_BLOCK,
    BC3_UNormBlock = VK_FORMAT_BC3_UNORM_BLOCK,
    BC3_SRGBBlock = VK_FORMAT_BC3_SRGB_BLOCK,
    BC4_UNormBlock = VK_FORMAT_BC4_UNORM_BLOCK,
    BC4_SNormBlock = VK_FORMAT_BC4_SNORM_BLOCK,
    BC5_UNormBlock = VK_FORMAT_BC5_UNORM_BLOCK,
    BC5_SNormBlock = VK_FORMAT_BC5_SNORM_BLOCK,
    BC6H_UFLOAT_BLOCK = VK_FORMAT_BC6H_UFLOAT_BLOCK,
    BC6H_SFLOAT_BLOCK = VK_FORMAT_BC6H_SFLOAT_BLOCK,
    BC7_UNormBlock = VK_FORMAT_BC7_UNORM_BLOCK,
    BC7_SRGBBlock = VK_FORMAT_BC7_SRGB_BLOCK,
    ETC2_R8G8B8_UNormBlock = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
    ETC2_R8G8B8_SRGBBlock = VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
    ETC2_R8G8B8A1_UNormBlock = VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
    ETC2_R8G8B8A1_SRGBBlock = VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
    ETC2_EAC_R8G8B8A8_UNormBlock = VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
    ETC2_EAC_R8G8B8A8_SRGBBlock = VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
    EAC_R11_UNormBlock = VK_FORMAT_EAC_R11_UNORM_BLOCK,
    EAC_R11_SNormBlock = VK_FORMAT_EAC_R11_SNORM_BLOCK,
    EAC_R11G11_UNormBlock = VK_FORMAT_EAC_R11G11_UNORM_BLOCK,
    EAC_R11G11_SNormBlock = VK_FORMAT_EAC_R11G11_SNORM_BLOCK,
    ASTC_4x4_UNormBlock = VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
    ASTC_4x4_SRGBBlock = VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
    ASTC_5x4_UNormBlock = VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
    ASTC_5x4_SRGBBlock = VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
    ASTC_5x5_UNormBlock = VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
    ASTC_5x5_SRGBBlock = VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
    ASTC_6x5_UNormBlock = VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
    ASTC_6x5_SRGBBlock = VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
    ASTC_6x6_UNormBlock = VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
    ASTC_6x6_SRGBBlock = VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
    ASTC_8x5_UNormBlock = VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
    ASTC_8x5_SRGBBlock = VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
    ASTC_8x6_UNormBlock = VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
    ASTC_8x6_SRGBBlock = VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
    ASTC_8x8_UNormBlock = VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
    ASTC_8x8_SRGBBlock = VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
    ASTC_10x5_UNormBlock = VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
    ASTC_10x5_SRGBBlock = VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
    ASTC_10x6_UNormBlock = VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
    ASTC_10x6_SRGBBlock = VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
    ASTC_10x8_UNormBlock = VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
    ASTC_10x8_SRGBBlock = VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
    ASTC_10x10_UNormBlock = VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
    ASTC_10x10_SRGBBlock = VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
    ASTC_12x10_UNormBlock = VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
    ASTC_12x10_SRGBBlock = VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
    ASTC_12x12_UNormBlock = VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
    ASTC_12x12_SRGBBlock = VK_FORMAT_ASTC_12x12_SRGB_BLOCK,

    // Depth formats
    D16_UNorm = VK_FORMAT_D16_UNORM,
    X8_D24_UNormPack32 = VK_FORMAT_X8_D24_UNORM_PACK32,
    D32_SFloat = VK_FORMAT_D32_SFLOAT,

    // Stencil formats
    S8_UInt = VK_FORMAT_S8_UINT,

    // Depth & stencil formats
    D16_UNorm_S8_UInt = VK_FORMAT_D16_UNORM_S8_UINT,
    D24_UNorm_S8_UInt = VK_FORMAT_D24_UNORM_S8_UINT,
    D32_SFloat_S8_UInt = VK_FORMAT_D32_SFLOAT_S8_UINT,
};

enum class FenceFlags : uint32_t
{
    CreateSignalled = VK_FENCE_CREATE_SIGNALED_BIT,
};

/// Specifies which statistics to track between `Grace::CommandBuffer::BeginQuery` and
/// `Grace::CommandBuffer::EndQuery`
enum class QueryStats : uint32_t
{
    // Queries the number of vertices processed by the input assembly stage
    InputAssemblyVertices = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT,
    // Queries the number of primitives processed by the input assembly stage
    InputAssemblyPrimitives = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT,
    // Queries the number of vertex shader invocations
    VertexShaderInvocations = VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT,
    // Queries the number of geometry shader invocations. In the case of instanced geometry
    // shaders, the count is incremented for each separate instanced invocation
    GeometryShaderInvocations = VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT,
    // Queries the number of primitives generated by geometry shader invocations
    GeometryShaderPrimitives = VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT,
    // Queries the number of primitives processed by the Primitive Clipping stage
    ClippingInvocations = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT,
    // Queries the number of primitives output by the Primitive Clipping stage
    ClippingPrimitives = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT,
    // Queries the number of fragment shader invocations
    FragmentShaderInvocations = VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT,
    // Queries the number of patches processed by the tessellation control shader
    TessellationCtrlShaderPatches = VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT,
    // Queries the number of invocations of the tessellation evaluation shader
    TessellationEvalShaderInvocations = VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT,
    // Queries the number of compute shader invocations
    ComputeShaderInvocations = VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT,
    // Queries the number of task shader invocations
    TaskShaderInvocations = VK_QUERY_PIPELINE_STATISTIC_TASK_SHADER_INVOCATIONS_BIT_EXT,
    // Queries the number of mesh shader invocations
    MeshShaderInvocations = VK_QUERY_PIPELINE_STATISTIC_MESH_SHADER_INVOCATIONS_BIT_EXT,
};
GRACE_ENUM_ENABLE_BITMASK_OR_OP(QueryStats);
GRACE_ENUM_ENABLE_BITMASK_AND_OP(QueryStats);

/// Specifies how and when query results are returned
enum class QueryResult : uint32_t
{
    // Query results will be written as an array of 64-bit unsigned integer values
    // instead of the default 32-bit unsigned integer values
    Bit64 = VK_QUERY_RESULT_64_BIT,
    // Forces a wait for each query’s status to become available before retrieving its results
    Wait = VK_QUERY_RESULT_WAIT_BIT,
    // Queries will be written along with an extra availability or status value directly
    // after the results of each query and interpreted as an unsigned integer
    WithAvailability = VK_QUERY_RESULT_WITH_AVAILABILITY_BIT,
    // For any query that is unavailable, an intermediate result
    // between zero and the final result value will be written for that query
    Partial = VK_QUERY_RESULT_PARTIAL_BIT,
    // Queries will be written along with an extra status value directly
    // after the results of each query and interpreted as an unsigned integer
    WithStatus = VK_QUERY_RESULT_WITH_STATUS_BIT_KHR,
};
GRACE_ENUM_ENABLE_BITMASK_OR_OP(QueryResult);
GRACE_ENUM_ENABLE_BITMASK_AND_OP(QueryResult);

} // namespace Grace
