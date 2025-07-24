#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout (location = 0) out vec2 outTexCoords;

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
} PushConst;

void main()
{
    Vertex v = PushConst.vbuffer.vertices[gl_VertexIndex];
    outTexCoords = v.uv;
    gl_Position = PushConst.mvp * vec4(v.position, 1.0);
}