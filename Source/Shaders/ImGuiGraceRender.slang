// grace entry VertEntry
// grace entry FragEntry

#include <Bindless.slang>

struct PC
{
    ImDrawVert* bufVertexAddr;
    uint textureId;
    uint samplerId;
    float2 scale;
    float2 translate;
};

[[vk::push_constant]]
PC PushConst;

struct FSInput {
    float4 position : SV_Position;
    float4 colour;
    float2 uv;
};

struct ImDrawVert
{
    float2 pos;
    float2 uv;
    uint col;
};

[shader("vertex")]
FSInput VertEntry(uint vertexId : SV_VulkanVertexID)
{
    const ImDrawVert v = PushConst.bufVertexAddr[vertexId];

    const float4 unpackedColour = float4(
        v.col >> 24,
        (v.col >> 16) & 255,
        (v.col >> 8) & 255,
        v.col & 255,
    ) / 255.0;

    FSInput output;
    output.position = float4(v.pos * PushConst.scale + PushConst.translate, 0.0, 1.0);
    output.colour = unpackedColour.abgr;
    output.uv = v.uv;
    return output;
}

[shader("fragment")]
float4 FragEntry(FSInput input) : SV_Target
{
    Texture2D t = uSampledTextures[PushConst.textureId];
    SamplerState s = uSamplers[PushConst.samplerId];
    return input.colour * t.Sample(s, input.uv);
}