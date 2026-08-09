#version 460

#include <Grace/Shaders/Bindless.glsl>

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) out vec4 outColour;
layout (location = 0) in vec2 inTexCoords;

GRACE_BINDLESS_REQUEST_SAMPLED_IMAGES();
GRACE_BINDLESS_REQUEST_SAMPLERS();

struct Vertex
{
    vec3 position;
    vec2 uv;
};

layout (scalar, buffer_reference) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout (scalar, push_constant) uniform PushConstants
{
    VertexBuffer vbuffer;
    mat4 mvp;
    uint textureIndex;
    uint linearWrapSamplerIndex;
} PushConst;

void main()
{
    vec4 c = texture(
        sampler2D(uGraceSampledImages[PushConst.textureIndex], uGraceSamplers[PushConst.linearWrapSamplerIndex]),
        inTexCoords
    );
    outColour = c;
}
