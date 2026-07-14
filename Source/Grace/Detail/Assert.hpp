#pragma once

#include <Grace/Detail/Logging.hpp>
#include <source_location>
#include <cstdlib>

// clang-format off
#define GRACE_ASSERT_MSG(assertion, msg)                                                                \
    do {                                                                                                \
        if (!(assertion))                                                                               \
        {                                                                                               \
            auto loc = std::source_location::current();                                                 \
            GRACE_ERROR("Assertion {} failed!", #assertion);                                            \
            GRACE_ERROR("{:>10} {}", "File:", loc.file_name());                                         \
            GRACE_ERROR("{:>10} {}", "Func:", loc.function_name());                                     \
            GRACE_ERROR("{:>10} {}", "Line:", loc.line());                                              \
            GRACE_ERROR("{:>10} {}", "Msg: ", msg);                                                     \
            std::abort();                                                                               \
        }                                                                                               \
    } while (0)                                                                                         \

#define GRACE_ASSERT(assertion)                                                                         \
    do {                                                                                                \
        if (!(assertion))                                                                               \
        {                                                                                               \
            auto loc = std::source_location::current();                                                 \
            GRACE_ERROR("Assertion {} failed!", #assertion);                                            \
            GRACE_ERROR("{:>10} {}", "File:", loc.file_name());                                         \
            GRACE_ERROR("{:>10} {}", "Func:", loc.function_name());                                     \
            GRACE_ERROR("{:>10} {}", "Line:", loc.line());                                              \
            std::abort();                                                                               \
        }                                                                                               \
    } while (0)                                                                                         \
// clang-format on
