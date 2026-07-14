#pragma once

#include <Grace/GraceApi.hpp>

#include <initializer_list>
#include <string>

namespace Grace::Ext
{

GRACE_API void CompileShaderSingle(const std::string& relativePath);

GRACE_API void CompileShaderMulti(std::initializer_list<std::string> relativePaths);

} // namespace Grace::Ext
