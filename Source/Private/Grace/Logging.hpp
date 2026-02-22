#pragma once

#include <spdlog/spdlog.h>
#include <string>

namespace Grace
{

inline const std::string kLoggerName = "Grace-Logger";

// clang-format off
#define GRACE_INFO(...) do { spdlog::get(kLoggerName)->info(__VA_ARGS__); } while (0)
#define GRACE_TRACE(...) do { spdlog::get(kLoggerName)->trace(__VA_ARGS__); } while (0)
#define GRACE_DEBUG(...) do { spdlog::get(kLoggerName)->debug(__VA_ARGS__); } while (0)
#define GRACE_WARN(...) do { spdlog::get(kLoggerName)->warn(__VA_ARGS__); } while (0)
#define GRACE_ERROR(...) do { spdlog::get(kLoggerName)->error(__VA_ARGS__); } while (0)
// clang-format on

} // namespace Grace
