#include "StreamCompaction.slang"

struct PredEvalFn : StreamCompactionPredicateEvaluationFn
{
    typealias PRED_TYPE = uint;
    typealias BUFFER_ELEM_TYPE = uint;

    bool EvaluatePredicate(const PRED_TYPE item)
    {
        return item > 0;
    }

    PRED_TYPE GetPredicateElement(BUFFER_ELEM_TYPE* src, const uint index)
    {
        return src[index];
    }
}

struct PC
{
    uint* sparseArrayBuffer;
    uint* compactedArrayBuffer;
    uint* metadataBuffer;
    uint sparseArrayBufferSize;
};

[[vk::push_constant]]
PC PushConst;

[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 globalThreadId : SV_DispatchThreadID,
          uint3 workgroupId : SV_GroupID,
          uint3 localThreadId : SV_GroupThreadID)
{
    PredEvalFn pef;
    StreamCompactionNonOrderPreserving<uint, PredEvalFn>(
            globalThreadId.x,
            localThreadId.x,
            PushConst.sparseArrayBuffer,
            PushConst.compactedArrayBuffer,
            0,
            PushConst.sparseArrayBufferSize,
            PushConst.metadataBuffer[0],
            pef
    );
}
