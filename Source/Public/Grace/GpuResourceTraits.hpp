#pragma once

#include <Grace/GpuResourceDescriptions.hpp>

#include <concepts>

namespace Grace
{

class Device;

struct GpuBindlessCompatibleTag
{};

template <typename T>
concept GpuManaged = std::movable<T> && !std::copyable<T> && requires(T r, Device* d, const GpuResourceDesc<T>& desc) {
    T(d, desc);
    { r.Exists() } -> std::same_as<bool>;
    r.VkHandle();
} ;

template <typename T>
concept GpuBindlessCompatible = std::is_base_of_v<GpuBindlessCompatibleTag, T>;

} // namespace Grace
