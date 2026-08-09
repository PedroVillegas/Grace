#pragma once

#include <Grace/GraceApi.hpp>

#include <initializer_list>
#include <string>

namespace Grace::Ext
{

GRACE_API void CompileSlang(
    const std::string& shader,
    const std::string& spirvDir,
    const std::string& compiler,
    std::initializer_list<std::string> compilerArgs);

GRACE_API void CompileSlang(
    std::initializer_list<std::string> shaders,
    const std::string& spirvDir,
    const std::string& compiler,
    std::initializer_list<std::string> compilerArgs);

GRACE_API void CompileGlsl(
    const std::string& shader,
    const std::string& spirvDir,
    const std::string& compiler,
    std::initializer_list<std::string> compilerArgs);

GRACE_API void CompileGlsl(
    std::initializer_list<std::string> shaders,
    const std::string& spirvDir,
    const std::string& compiler,
    std::initializer_list<std::string> compilerArgs);

} // namespace Grace::Ext
