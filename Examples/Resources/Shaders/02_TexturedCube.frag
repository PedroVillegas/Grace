#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) out vec4 outColour;
layout(location = 0) in vec2 inTexCoords;

layout(set = 0, binding = 1) uniform texture2D uTextures[];
layout(set = 0, binding = 3) uniform sampler uSamplers[];

struct Vertex
{
    vec3 position;
    vec2 uv;
};

layout(scalar, buffer_reference) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout(scalar, push_constant) uniform PushConstants
{
    VertexBuffer vbuffer;
    mat4 mvp;
    uint textureIndex;
    uint linearWrapSamplerIndex;
} PushConst;

void main()
{
    vec4 c = texture(sampler2D(uTextures[PushConst.textureIndex], uSamplers[PushConst.linearWrapSamplerIndex]), inTexCoords);
    outColour = c;
}
