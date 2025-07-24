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

uint hash(uint a)
{
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^uint(0xc761c23c)) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^uint(0xb55a4f09)) ^ (a>>16);
    return a;
}

void main()
{
    uint hashed = hash(gl_PrimitiveID);
    vec3 cl = vec3(float(hashed & 255U), float((hashed >> 8) & 255U), float((hashed >> 16) & 255U)) / 255.0;
    outColour = vec4(cl, 1.0);
}
