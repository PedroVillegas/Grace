#pragma once

#include <Grace/Detail/StackAllocator.hpp>

#include <vector>

namespace Grace
{

template <typename T>
using ScratchVector = std::vector<T, StackAllocator<T>>;

} // namespace Grace
