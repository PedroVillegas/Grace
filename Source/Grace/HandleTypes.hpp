#pragma once

#include <stdint.h>

namespace Grace
{

const uint32_t INVALID_HANDLE = (1u << 31) - 1;
const uint32_t INVALID_VALIDATOR = (1u << 31) - 1;

template <typename ResourceType>
struct Handle;

class Buffer;
class Image;
class Sampler;
struct Pipeline;

using BufferHandle = Handle<Buffer>;
using ImageHandle = Handle<Image>;
using SamplerHandle = Handle<Sampler>;
using PipelineHandle = Handle<Pipeline>;
using PipelineLayoutHandle = Handle<PipelineLayout>;

template <typename ResourceType>
struct Handle
{
    Handle() = default;

    Handle(uint32_t UUID, uint32_t Validator)
    {
        handle = UUID;
        validator = Validator;
    }

    [[nodiscard]] bool HasValidHandle() const
    {
        return handle != INVALID_HANDLE;
    }

    uint32_t handle = INVALID_HANDLE;
    uint32_t validator = INVALID_VALIDATOR;
};

} // namespace Grace
