#include <chrono>
#include <random>
#include <iostream>

#include <Grace/Grace.hpp>

#define VERIFY_RESULTS 1

int main()
{
    const Grace::DeviceDesc deviceDesc = {
        .maxImageDescriptors = 65535,
        .maxSamplerDescriptors = 65535,
        .maxBufferDescriptors = 65535,
        .framesInFlight = 1,
        .queryGroupDesc = {
            .pipelineStatisticsFlags = Grace::QueryStats::ComputeShaderInvocations,
        },
        .pGlfwWindow = nullptr,
    };

    Grace::Context gpuContext({ .deviceConfig = deviceDesc });
    Grace::Device* pDevice = gpuContext.GetDevicePtr();

    // Grab a Command Pool for the Graphics Queue and allocate a command buffer from it
    Grace::CommandPool* pCmdPool = pDevice->GetCommandPool(Grace::QueueFamily::Graphics, "Example04::pCmdPool");
    Grace::CommandBuffer cmd = pCmdPool->GetOrAllocateCommandBuffer();

    const Grace::PipelineHandle streamCompactionPipeline = pDevice->CreatePipeline({
        .name = "Example04::streamCompactionPipeline",
        .shaders = { { .stage = Grace::ShaderStage::Compute, .name = "04_StreamCompaction.slang.spv" } },
        .layout = pDevice->GetSolePipelineLayout(),
    });

    const Grace::PipelineHandle streamCompactionNonOrderPreservingPipeline = pDevice->CreatePipeline({
        .name = "Example04::streamCompactionNonOrderPreservingPipeline",
        .shaders = { { .stage = Grace::ShaderStage::Compute,
                       .name = "04_StreamCompactionNonOrderPreserving.slang.spv" } },
        .layout = pDevice->GetSolePipelineLayout(),
    });

    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution distrib(0, 4);

    const uint32_t WorkgroupSize = 256;

    const uint32_t arraySize = 1024 << 10;
    std::vector<uint32_t> sparseArray(arraySize);
    for (uint32_t& i : sparseArray)
    {
        i = distrib(gen);
    }

    const uint32_t dispatchSize = std::ceil(sparseArray.size() / float(WorkgroupSize * 16));

    uint32_t compactedArraySize = 0;
    const Grace::BufferHandle metadataBuffer = pDevice->CreateBuffer({
        .name = "Example04::metadataBuffer",
        .usage =
            Grace::BufferUsage::StorageBuffer | Grace::BufferUsage::DeviceAddress | Grace::BufferUsage::TransferDst,
        .allocFlags = 0,
        .size = sizeof(uint32_t),
        .data = &compactedArraySize,
    });

    const Grace::BufferHandle sparseArrayBuffer = pDevice->CreateBuffer({
        .name = "Example04::sparseArrayBuffer",
        .usage = Grace::BufferUsage::StorageBuffer | Grace::BufferUsage::DeviceAddress | Grace::BufferUsage::TransferDst
               | Grace::BufferUsage::TransferSrc,
        .allocFlags = 0,
        .size = sparseArray.size() * sizeof(uint32_t),
        .data = sparseArray.data(),
    });

    const Grace::BufferHandle compactedArrayBuffer = pDevice->CreateBuffer({
        .name = "Example04::compactedArrayBuffer",
        .usage = Grace::BufferUsage::StorageBuffer | Grace::BufferUsage::DeviceAddress | Grace::BufferUsage::TransferDst
               | Grace::BufferUsage::TransferSrc,
        .allocFlags = 0,
        .size = sparseArray.size() * sizeof(uint32_t),
        .data = nullptr,
    });

    const Grace::BufferHandle intermediateBuffer = pDevice->CreateBuffer({
        .name = "Example04::intermediateBuffer",
        .usage = Grace::BufferUsage::StorageBuffer | Grace::BufferUsage::TransferDst,
        .allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .size = sparseArray.size() * sizeof(uint32_t),
        .data = nullptr,
    });

    // Reset command pool, which will reset all command buffers allocated from it too
    pCmdPool->Reset();

    pDevice->UpdateBindlessDescriptorSet();

    // Now that the command buffer has been reset, we can start recording for the subsequent frame
    cmd.BeginRecording();
    cmd.ResetQueryPoolFullRange<Grace::QueryType::Timestamp>(0);

    /* Record commands */

    cmd.BeginDebugLabel("Stream Compaction Pass");

    struct PushConsts
    {
        uint64_t sparseArrayBuffer;
        uint64_t compactedArrayBuffer;
        uint64_t metadataBuffer;
        uint32_t sparseArrayBufferSize;
    } pc;

    pc.sparseArrayBuffer = pDevice->GetBuffer(sparseArrayBuffer).GetBDA();
    pc.compactedArrayBuffer = pDevice->GetBuffer(compactedArrayBuffer).GetBDA();
    pc.metadataBuffer = pDevice->GetBuffer(metadataBuffer).GetBDA();
    pc.sparseArrayBufferSize = sparseArray.size();

    cmd.PushConstants(pDevice->GetSolePipelineLayout(), sizeof(pc), &pc);
    cmd.BindDescriptorSets(
        Grace::PipelineBindPoint::Compute, pDevice->GetSolePipelineLayout(), { pDevice->GetSoleDescriptorSet() });
    // cmd.BindPipeline(streamCompactionPipeline);
    cmd.BindPipeline(streamCompactionNonOrderPreservingPipeline);

    cmd.WriteTimestamp("Compaction Pass Begin", Grace::PipelineStage::ComputeShader, 0);
    cmd.Dispatch(dispatchSize);
    cmd.WriteTimestamp("Compaction Pass End", Grace::PipelineStage::ComputeShader, 0);

    cmd.EndDebugLabel();

    cmd.AddMemoryBarrier({ Grace::AccessType::ComputeShaderWrite }, { Grace::AccessType::CopyRead });
    cmd.PipelineBarrier();

    cmd.CopyBuffer(compactedArrayBuffer, intermediateBuffer);

    // Finish recording for the command buffer for this frame
    cmd.EndRecording();

    // Submit the command buffer
    pDevice->SubmitAndWait(Grace::QueueFamily::Graphics, cmd);

#if VERIFY_RESULTS
    std::unordered_map<uint32_t, uint32_t> refElCounts;
    std::unordered_map<uint32_t, uint32_t> gpuElCounts;
    uint32_t refSize = 0;
    for (uint32_t i = 0; i < sparseArray.size(); i++)
    {
        if (sparseArray[i] == 0)
            continue;
        refElCounts[sparseArray[i]]++;
        refSize++;
    }

    uint32_t* intBufferPtr =
        (uint32_t*) pDevice->GetBuffer(intermediateBuffer).GetAllocationInfo().allocationInfo.pMappedData;
    for (uint32_t i = 0; i < refSize; i++)
    {
        gpuElCounts[*(intBufferPtr + i)]++;
    }

    std::cout << "RefElCounts\n";
    for (const auto& [k, v] : refElCounts)
    {
        std::cout << k << " = " << v << std::endl;
    }

    std::cout << "GpuElCounts\n";
    for (const auto& [k, v] : gpuElCounts)
    {
        std::cout << k << " = " << v << std::endl;
    }
#endif

    const Grace::TimestampQueryGroup& tqg =
        pDevice->GetQueryPoolResults<Grace::QueryType::Timestamp>(0, 0, Grace::QueryResult::Wait);

    const double timeToCompact =
        tqg.Duration<Grace::TimestampUnits::Milliseconds>("Compaction Pass Begin", "Compaction Pass End");
    std::cout << "\nTime to compact: " << timeToCompact << "ms" << std::endl;

    return 0;
}
