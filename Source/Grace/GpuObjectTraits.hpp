#pragma once

#include <Grace/GraceApi.hpp>

#include <concepts>

namespace Grace
{

class Device;

namespace SemaphoreType
{

struct GRACE_API SemaphoreTypeTag
{};

struct GRACE_API Binary : SemaphoreTypeTag
{};

struct GRACE_API Timeline : SemaphoreTypeTag
{};

} // namespace SemaphoreType

template <typename T>
concept SemaphoreVariant = std::derived_from<T, SemaphoreType::SemaphoreTypeTag>;

namespace PipelineType
{

struct GRACE_API PipelineTypeTag
{};

struct GRACE_API Graphics : PipelineTypeTag
{};

struct GRACE_API Compute : PipelineTypeTag
{};

} // namespace PipelineType

template <typename T>
concept PipelineVariant = std::derived_from<T, PipelineType::PipelineTypeTag>;

struct GRACE_API GpuBindlessCompatibleTag
{};

template <typename T>
concept GpuBindlessCompatible = std::is_base_of_v<GpuBindlessCompatibleTag, T>;

} // namespace Grace
