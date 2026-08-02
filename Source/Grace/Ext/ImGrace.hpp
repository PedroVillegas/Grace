#pragma once

#include <imgui/imgui.h>
#include <string>

#include <Grace/GraceApi.hpp>
#include <Grace/GpuObjectFwd.hpp>
#include <Grace/CommandGroup.hpp>

namespace Grace
{

class Device;

class GRACE_API ImGrace
{
public:
    explicit ImGrace(Device* gpu);
    void Draw(ImDrawData* drawData, CommandBuffer& cmd);

private:
    void CreateOrResizeBuffer(BufferHandle& buf,
                              const std::string& name,
                              size_t sizeInBytes,
                              BufferUsage usage);

    void UpdateTexture(ImTextureData* tex);

    ImageHandle ImTexAsGraceImageHandle(ImTextureID tex);

private:
    struct PushConst
    {
        uint64_t bufVertexAddr;
        uint32_t textureId;
        uint32_t samplerId;
        float scalex, scaley;
        float translatex, translatey;
    } mPushConst = {};

    Device* mGpu = nullptr;
    GraphicsPipelineHandle mPlRenderDrawData = {};
    BufferHandle mBufVertex = {};
    BufferHandle mBufIndex = {};
    SamplerHandle mSampler = {};
    std::string mBufVertexName = "ImGrace.BufVertex";
    std::string mBufIndexName = "ImGrace.BufIndex";
};

} // namespace Grace
