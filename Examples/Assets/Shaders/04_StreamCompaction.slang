struct PC
{
    uint* sparseArrayBuffer;
    uint sparseArrayBufferSize;
};

[[vk::push_constant]]
PC PushConst;

groupshared uint[1024] smemData;
groupshared uint[1024] smemPartialSums;
groupshared uint[1024] smemOut;

[shader("compute")]
[numthreads(1024, 1, 1)]
void main(uint2 id : SV_DispatchThreadID)
{
    uint N = PushConst.sparseArrayBufferSize;
    if (id.x >= N) return;

    uint input = PushConst.sparseArrayBuffer[id.x];
    smemData[id.x] = input;

    GroupMemoryBarrierWithWaveSync();

    uint sum = 0;
    sum = (smemData[id.x] == 0) ? 0 : 1;
    smemPartialSums[id.x] = WavePrefixSum(sum);

    // Wait for smemPartialSums to be filled completely
    GroupMemoryBarrierWithWaveSync();

    uint partialSumOfWave = smemPartialSums[N - 1];

    if (smemData[id.x] > 0)
    {
        PushConst.sparseArrayBuffer[smemPartialSums[id.x]] = smemData[id.x];
    }
}
