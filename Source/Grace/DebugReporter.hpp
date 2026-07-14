#pragma once

#include <Grace/GraceApi.hpp>

#include <vulkan/vulkan.h>

namespace Grace
{

class GRACE_API DebugReporter
{
public:
    static void Check(VkResult result);
};

} // namespace Grace
