#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <Grace/Device.hpp>
#include <Grace/GraceApi.hpp>

struct GLFWwindow;

namespace Grace
{

struct ContextDesc
{
    std::vector<const char*> extensions;
    DeviceDesc deviceConfig;
};

class GRACE_API Context
{
public:
    ~Context();
    Context() = default;
    explicit Context(const ContextDesc& desc);

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    Context(Context&&) noexcept = delete;
    Context& operator=(Context&&) noexcept = delete;

    GRACE_NODISCARD Device* GetDevicePtr();

    GRACE_NODISCARD VkInstance& GetInstance();

private:
    VkInstance mInstance = nullptr;
    std::unique_ptr<Device> mDevice = {};
};

} // namespace Grace
