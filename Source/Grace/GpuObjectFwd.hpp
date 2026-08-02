#pragma once

#include <Grace/GpuObjectTraits.hpp>

namespace Grace
{

namespace SemaphoreType
{

struct Binary;
struct Timeline;

} // namespace SemaphoreType

namespace PipelineType
{

struct Graphics;
struct Compute;

} // namespace PipelineType

template <typename T>
class GpuHandle;

class Buffer;
class Image;
class Sampler;
template <PipelineVariant T>
class Pipeline;
class PipelineLayout;
class Fence;
template <SemaphoreVariant T>
class Semaphore;

using BufferHandle = GpuHandle<Buffer>;
using ImageHandle = GpuHandle<Image>;
using SamplerHandle = GpuHandle<Sampler>;
using GraphicsPipelineHandle = GpuHandle<Pipeline<PipelineType::Graphics>>;
using ComputePipelineHandle = GpuHandle<Pipeline<PipelineType::Compute>>;
using PipelineLayoutHandle = GpuHandle<PipelineLayout>;
using FenceHandle = GpuHandle<Fence>;
using BinarySemaphoreHandle = GpuHandle<Semaphore<SemaphoreType::Binary>>;
using TimelineSemaphoreHandle = GpuHandle<Semaphore<SemaphoreType::Timeline>>;

} // namespace Grace
