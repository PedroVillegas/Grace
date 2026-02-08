#pragma once

#include <Private/Grace/StackAllocator.hpp>

#include <vector>

namespace Grace
{

template <typename T>
using ScratchVector = std::vector<T, StackAllocator<T>>;

} // namespace Grace
