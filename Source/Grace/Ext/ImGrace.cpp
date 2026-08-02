#include <Grace/Ext/ImGrace.hpp>
#include <Grace/Device.hpp>
#include <Grace/GpuBuffer.hpp>
#include <Grace/GpuSampler.hpp>
#include <vk_mem_alloc.h>
#include <algorithm>

// https://github.com/ocornut/imgui/blob/master/docs/BACKENDS.md#writing-your-own-backend

namespace Grace
{

ImGrace::ImGrace(Device* gpu) : mGpu(gpu)
{
    mPlRenderDrawData = mGpu->Create<GraphicsPipeline>({
        .name = "ImGuiGrace.PlRenderDrawData",
        .shaders = {
            ShaderDesc(ShaderStage::Vertex, GRACE_INTERNAL_SPV "/ImGuiGraceRender.slang.VertEntry.spv", ShaderFlags::AbsolutePath),
            ShaderDesc(ShaderStage::Fragment, GRACE_INTERNAL_SPV "/ImGuiGraceRender.slang.FragEntry.spv", ShaderFlags::AbsolutePath),
        },
        .graphicsState = GraphicsState({
            .colourAttachmentFormats = { Format::RGBA8_SRGB },
            .topology = Topology::TriangleList,
            .polygonMode = PolygonMode::Fill,
            .cullMode = CullMode::None,
            .frontFace = FrontFace::Clockwise,
            .multisample = MultisampleLevel::x1,
            .colorBlendMode = ColorBlendMode::AlphaBlend,
            .depthStencilUsage = DepthStencilUsage::None,
        }),
        .layout = mGpu->GetSolePipelineLayout(),
    });

    mSampler = mGpu->Create<Sampler>({
        .minFilter = Filter::Linear,
        .magFilter = Filter::Linear,
        .addressMode = SamplerAddressMode::ClampToEdge,
        .mipmapMode = SamplerMipmapMode::Linear,
    });
}

void ImGrace::Draw(ImDrawData* drawData, CommandBuffer& cmd)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    const int fbWidth = static_cast<int>(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    const int fbHeight = static_cast<int>(drawData->DisplaySize.y * drawData->FramebufferScale.y);
    if (fbWidth <= 0 || fbHeight <= 0 || drawData->TotalVtxCount <= 0)
    {
        return;
    }

    // Catch up with texture updates. Most of the time, the list will have 1 element with an OK status, aka nothing to do.
    // (This almost always points to ImGui::GetPlatformIO().Textures[] but is part of ImDrawData to allow overriding or disabling texture updates).
    if (drawData->Textures != nullptr)
        for (ImTextureData* tex : *drawData->Textures)
            if (tex->Status != ImTextureStatus_OK)
                UpdateTexture(tex);

    mGpu->UpdateBindlessDescriptorSet();

    {
        // Create or resize the vertex/index buffers
        const uint64_t oldVertexSizeBytes =
            mBufVertex.Exists() ? mGpu->Get<Buffer>(mBufVertex).GetAllocationInfo().allocationInfo.size : 0;
        const uint64_t oldIndexSizeBytes =
            mBufIndex.Exists() ? mGpu->Get<Buffer>(mBufIndex).GetAllocationInfo().allocationInfo.size : 0;
        const uint64_t newVertexSizeBytes = drawData->TotalVtxCount * sizeof(ImDrawVert);
        const uint64_t newIndexSizeBytes = drawData->TotalIdxCount * sizeof(ImDrawIdx);

        if (!mBufVertex.Exists() || oldVertexSizeBytes < newVertexSizeBytes)
        {
            CreateOrResizeBuffer(mBufVertex,
                                 mBufVertexName,
                                 newVertexSizeBytes,
                                 BufferUsage::DeviceAddress | BufferUsage::StorageBuffer);
        }

        if (!mBufIndex.Exists() || oldIndexSizeBytes < newIndexSizeBytes)
        {
            CreateOrResizeBuffer(mBufIndex, mBufIndexName, newIndexSizeBytes, BufferUsage::IndexBuffer);
        }

        // Upload vertex/index data into a single contiguous GPU buffer
        ImDrawVert* vertexDest = nullptr;
        ImDrawIdx* indexDest = nullptr;

        DebugReporter::Check(vmaMapMemory(mGpu->GetVmaHandle(),
                                                 mGpu->Get<Buffer>(mBufVertex).GetAllocation(),
                                                 reinterpret_cast<void**>(&vertexDest)));
        DebugReporter::Check(vmaMapMemory(mGpu->GetVmaHandle(),
                                                 mGpu->Get<Buffer>(mBufIndex).GetAllocation(),
                                                 reinterpret_cast<void**>(&indexDest)));

        for (const ImDrawList* drawList : drawData->CmdLists)
        {
            memcpy(vertexDest, drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(indexDest, drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
            vertexDest += drawList->VtxBuffer.Size;
            indexDest += drawList->IdxBuffer.Size;
        }

        std::array allocations = {
            mGpu->Get<Buffer>(mBufVertex).GetAllocation(),
            mGpu->Get<Buffer>(mBufIndex).GetAllocation(),
        };
        DebugReporter::Check(
            vmaFlushAllocations(mGpu->GetVmaHandle(), allocations.size(), allocations.data(), nullptr, nullptr));

        vmaUnmapMemory(mGpu->GetVmaHandle(), mGpu->Get<Buffer>(mBufVertex).GetAllocation());
        vmaUnmapMemory(mGpu->GetVmaHandle(), mGpu->Get<Buffer>(mBufIndex).GetAllocation());
    }

    // cmd.AddMemoryBarrier({ AccessType::HostWrite },
    //                      { AccessType::IndexBuffer, AccessType::AnyShaderStorageRead });
    // cmd.PipelineBarrier();

    // Bind pipeline & index buffer:
    cmd.BindPipeline(mPlRenderDrawData);
    const IndexType indexType =
        sizeof(ImDrawIdx) == sizeof(uint32_t) ? IndexType::UInt32 : IndexType::UInt16;
    cmd.BindIndexBuffer(mBufIndex, 0, indexType);

    // Setup scale and translation:
    // Our visible imgui space lies from drawData->DisplayPps (top left) to drawData->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    mPushConst.bufVertexAddr = mGpu->Get<Buffer>(mBufVertex).GetBDA();
    mPushConst.samplerId = mGpu->Get<Sampler>(mSampler).GetSamplerId();
    mPushConst.scalex = 2.0f / drawData->DisplaySize.x;
    mPushConst.scaley = 2.0f / drawData->DisplaySize.y;
    mPushConst.translatex = -1.0f - drawData->DisplayPos.x * mPushConst.scalex;
    mPushConst.translatey = -1.0f - drawData->DisplayPos.y * mPushConst.scaley;
    cmd.PushConstants(mGpu->GetSolePipelineLayout(), &mPushConst);

    // Setup viewport:
    cmd.SetViewport({ VkViewport {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(fbWidth),
        .height = static_cast<float>(fbHeight),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    } });

    // Setup sampler
    cmd.BindDescriptorSets(
        PipelineBindPoint::Graphics, mGpu->GetSolePipelineLayout(), { mGpu->GetSoleDescriptorSet() });

    // Will project scissor/clipping rectangles into framebuffer space
    const ImVec2 clipOffset = drawData->DisplayPos;      // (0,0) unless using multi-viewports
    const ImVec2 clipScale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    ImageHandle lastImageId = {};
    int32_t globalVertexOffset = 0;
    int32_t globalIndexOffset = 0;
    for (const ImDrawList* drawList : drawData->CmdLists)
    {
        for (int32_t cmd_i = 0; cmd_i < drawList->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &drawList->CmdBuffer[cmd_i];
            // Project scissor/clipping rectangles into framebuffer space
            ImVec2 clipMin = {
                (pcmd->ClipRect.x - clipOffset.x) * clipScale.x,
                (pcmd->ClipRect.y - clipOffset.y) * clipScale.y,
            };
            ImVec2 clipMax = {
                (pcmd->ClipRect.z - clipOffset.x) * clipScale.x,
                (pcmd->ClipRect.w - clipOffset.y) * clipScale.y,
            };

            // Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
            clipMin.x = std::max(clipMin.x, 0.0f);
            clipMin.y = std::max(clipMin.y, 0.0f);
            clipMax.x = std::min(clipMax.x, static_cast<float>(fbWidth));
            clipMax.y = std::min(clipMax.y, static_cast<float>(fbHeight));
            if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                continue;

            // Apply scissor/clipping rectangle
            cmd.SetScissor({ VkRect2D {
                .offset = {
                    .x = static_cast<int32_t>(clipMin.x),
                    .y = static_cast<int32_t>(clipMin.y),
                },
                .extent = {
                    .width = static_cast<uint32_t>(clipMax.x - clipMin.x),
                    .height = static_cast<uint32_t>(clipMax.y - clipMin.y),
                },
            } });

            // Bind DescriptorSets for image view (font or user texture) and samplers
            ImageHandle imageView = ImTexAsGraceImageHandle(pcmd->GetTexID());
            if (imageView != lastImageId)
            {
                mPushConst.textureId = mGpu->Get<Image>(imageView).GetSampledImgId();
                cmd.PushConstants(mGpu->GetSolePipelineLayout(), &mPushConst);
            }
            lastImageId = imageView;

            // Draw
            cmd.DrawIndexed(pcmd->ElemCount,
                            1,
                            globalIndexOffset + pcmd->IdxOffset,
                            globalVertexOffset + static_cast<int32_t>(pcmd->VtxOffset),
                            0);
        }
        globalIndexOffset += drawList->IdxBuffer.Size;
        globalVertexOffset += drawList->VtxBuffer.Size;
    }

    // Note: at this point both vkCmdSetViewport() and vkCmdSetScissor() have been called.
    // Our last values will leak into user/application rendering IF:
    // - Your app uses a pipeline with VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR dynamic state
    // - And you forgot to call vkCmdSetViewport() and vkCmdSetScissor() yourself to explicitly set that state.
    // If you use VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR you are responsible for setting the values before rendering.
    // In theory we should aim to backup/restore those values but I am not sure this is possible.
    // We perform a call to vkCmdSetScissor() to set back a full viewport which is likely to fix things for 99% users but technically this is not perfect. (See github #4644)
    cmd.SetScissor({ VkRect2D {
        .offset = { .x = 0, .y = 0 },
        .extent = {
            .width = static_cast<uint32_t>(fbWidth),
            .height = static_cast<uint32_t>(fbHeight),
        },
    } });
}

void ImGrace::CreateOrResizeBuffer(BufferHandle& buf,
                                   const std::string& name,
                                   const size_t sizeInBytes,
                                   const BufferUsage usage)
{
    if (buf.Exists())
    {
        mGpu->Free<Buffer>(buf);
    }

    buf = mGpu->Create<Buffer>({
        .name = name.c_str(),
        .usage = usage,
        .allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .size = sizeInBytes,
        .data = nullptr,
    });
}

void ImGrace::UpdateTexture(ImTextureData* tex)
{
    if (tex->Status == ImTextureStatus_OK)
        return;

    if (tex->Status == ImTextureStatus_WantCreate)
    {
        const ImageHandle img = mGpu->Create<Image>(
            {
                .name = "ImGui Texture",
                .dimensions = UInt3(tex->Width, tex->Height, 1),
                .format = Format::RGBA8_UNorm,
                .usage = ImageUsage::SampledImage | ImageUsage::TransferDst,
                .access = AccessType::General,
                .size = 0,
                .data = nullptr,
                .mipmapped = false,
            },
            GpuHandleFlags::DisableRefCount);

        // Store identifiers
        tex->SetTexID(static_cast<ImTextureID>(img.AsUInt64()));
    }

    ImageHandle reconstructedHandle = ImTexAsGraceImageHandle(tex->GetTexID());
    if (tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates)
    {
        // Update full texture or selected blocks. We only ever write to textures regions which have never been used before!
        // This backend choose to use tex->UpdateRect but you can use tex->Updates[] to upload individual regions.
        // We could use the smaller rect on _WantCreate but using the full rect allows us to clear the texture.
        const int upload_x = (tex->Status == ImTextureStatus_WantCreate) ? 0 : tex->UpdateRect.x;
        const int upload_y = (tex->Status == ImTextureStatus_WantCreate) ? 0 : tex->UpdateRect.y;
        const int upload_w = (tex->Status == ImTextureStatus_WantCreate) ? tex->Width : tex->UpdateRect.w;
        const int upload_h = (tex->Status == ImTextureStatus_WantCreate) ? tex->Height : tex->UpdateRect.h;

        // Create the Upload Buffer:

        const uint64_t uploadPitch = upload_w * tex->BytesPerPixel;
        const uint64_t uploadSize = upload_h * uploadPitch;

        BufferHandle bufUpload = mGpu->Create<Buffer>({
            .name = "ImGui Texture Upload Buffer",
            .usage = BufferUsage::TransferSrc,
            .allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .size = uploadSize,
            .data = nullptr,
        });

        char* map = nullptr;

        // Upload to Buffer:
        DebugReporter::Check(vmaMapMemory(
            mGpu->GetVmaHandle(), mGpu->Get<Buffer>(bufUpload).GetAllocation(), reinterpret_cast<void**>(&map)));

        for (int y = 0; y < upload_h; y++)
            memcpy(map + uploadPitch * y, tex->GetPixelsAt(upload_x, upload_y + y), static_cast<size_t>(uploadPitch));

        std::array allocations = { mGpu->Get<Buffer>(bufUpload).GetAllocation() };
        DebugReporter::Check(
            vmaFlushAllocations(mGpu->GetVmaHandle(), allocations.size(), allocations.data(), nullptr, nullptr));

        vmaUnmapMemory(mGpu->GetVmaHandle(), mGpu->Get<Buffer>(bufUpload).GetAllocation());

        CommandBuffer& cmd = mGpu->BeginSingleTimeCommands();

        cmd.AddMemoryBarrier({ AccessType::HostWrite }, { AccessType::AnyTransferRead });
        cmd.AddImageBarrier(reconstructedHandle, { AccessType::HostWrite }, { AccessType::AnyTransferWrite });
        cmd.PipelineBarrier();

        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = upload_w;
        region.imageExtent.height = upload_h;
        region.imageExtent.depth = 1;
        region.imageOffset.x = upload_x;
        region.imageOffset.y = upload_y;
        cmd.CopyBufferToImageRegions(bufUpload, reconstructedHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { region });

        cmd.AddImageBarrier(reconstructedHandle, { AccessType::AnyTransferWrite }, { AccessType::General });
        cmd.PipelineBarrier();

        mGpu->EndAndSubmitSingleTimeCommands();

        mGpu->Free<Buffer>(bufUpload);
        tex->SetStatus(ImTextureStatus_OK);
    }

    if (tex->Status == ImTextureStatus_WantDestroy /* && tex->UnusedFrames >= (int) bd->VulkanInitInfo.ImageCount */)
        mGpu->FreeDeferred<Image>(reconstructedHandle);
}

ImageHandle ImGrace::ImTexAsGraceImageHandle(ImTextureID tex)
{
    uint32_t handle = tex >> 32u;
    uint32_t gen = tex & 0xffffffffu;
    bool refcounted = (gen >> 31u) == 1u;
    return ImageHandle(handle, gen, refcounted);
}

} // namespace Grace
