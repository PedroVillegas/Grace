#pragma once

#include <filesystem>

#include <Grace/GraceApi.hpp>
#include <Grace/Detail/Macros.hpp>
#include <Grace/HelperFunctions.hpp>
#include <Grace/GpuObjectDesc.hpp>

namespace Grace
{

class Device;

class GRACE_API PipelineLayout
{
public:
    ~PipelineLayout();
    PipelineLayout() = default;
    PipelineLayout(Device* pDevice, const GpuObjectDesc<PipelineLayout>& desc);

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkPipelineLayout handle more than once
    PipelineLayout(const PipelineLayout&) = delete;
    PipelineLayout& operator=(const PipelineLayout&) = delete;

    PipelineLayout(PipelineLayout&& other) noexcept;
    PipelineLayout& operator=(PipelineLayout&& other) noexcept;

    GRACE_NODISCARD bool Exists() const;

    GRACE_NODISCARD VkPipelineLayout VkHandle() const;

private:
    Device* mDevice = nullptr;
    VkPipelineLayout mPipelineLayout = nullptr;
};

template <PipelineVariant T>
class GRACE_API Pipeline
{
public:
    ~Pipeline();
    Pipeline() = default;

    Pipeline(Device* device, const GpuObjectDesc<Pipeline<T>>& desc);

    // Copy constructions/assignments are prohibited to stop destructor trying to
    // destroy the same VkPipeline handle more than once
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    Pipeline(Pipeline&& other) noexcept;
    Pipeline& operator=(Pipeline&& other) noexcept;

    GRACE_NODISCARD bool Exists() const;

    GRACE_NODISCARD VkPipeline VkHandle() const;

    GRACE_NODISCARD VkPipelineBindPoint BindPoint() const;

private:
    Device* mDevice = nullptr;
    VkPipeline mPipeline = nullptr;
};

using GraphicsPipeline = Pipeline<PipelineType::Graphics>;
using ComputePipeline = Pipeline<PipelineType::Compute>;

} // namespace Grace
