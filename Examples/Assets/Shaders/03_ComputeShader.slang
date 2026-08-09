#include <Grace/Shaders/Bindless.slang>

GRACE_BINDLESS_REQUEST_STORAGE_IMAGES();

struct PC
{
    uint TextureId;
};

[[vk::push_constant]]
PC PushConst;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint2 id : SV_DispatchThreadID)
{
    RWTexture2D<float4> renderTexture = uGraceStorageImages[PushConst.TextureId];

    uint2 renderDims;
    renderTexture.GetDimensions(renderDims.x, renderDims.y);

    if (any(id >= renderDims)) return;

    float2 uv = float2(id) / renderDims;

    float3 c = float3(uv, 0.5);

    renderTexture[id] = float4(pow(c, float3(1.0 / 2.2)), 1.0);
}
