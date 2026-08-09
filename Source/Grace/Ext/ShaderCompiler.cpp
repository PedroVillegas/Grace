#include <Grace/Ext/ShaderCompiler.hpp>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include <iostream>

namespace Grace::Ext
{

static std::string SingleStringCompilerArgs(const std::initializer_list<std::string>& args)
{
    // TODO: fix potential overflow
    std::string slangcArgs;
    slangcArgs.reserve(2048);
    slangcArgs.append("'");
    for (const std::string& arg : args)
    {
        slangcArgs.append(arg);
        slangcArgs.append(" ~ ");
    }
    slangcArgs.append("-I ~ " GRACE_INTERNAL_SHADER_INCLUDE);
    slangcArgs.append("'");
    slangcArgs.shrink_to_fit();
    return slangcArgs;
}

static std::string SingleStringShaders(const std::initializer_list<std::string>& shaders)
{
    std::string shadersStr;
    shadersStr.reserve(2048);
    shadersStr.append("'");
    for (uint32_t i = 0; const std::string& sh : shaders)
    {
        shadersStr.append(sh);
        if (i < shaders.size() - 1)
        {
            shadersStr.append(" ~ ");
        }
        i++;
    }
    shadersStr.append("'");
    shadersStr.shrink_to_fit();
    return shadersStr;
}

void CompileSlang(const std::string& shader,
                  const std::string& spirvDir,
                  const std::string& compiler,
                  std::initializer_list<std::string> compilerArgs)
{
    std::string slangcArgs = SingleStringCompilerArgs(compilerArgs);

    const std::array<const char*, 9> argv = {
        GRACE_PY_SHADER_COMP_SCRIPT,
        "--shaders", shader.c_str(),
        "--spirv-dir", spirvDir.c_str(),
        "--slangc", compiler.c_str(),
        "--slangc-args", slangcArgs.c_str(),
    };

    const pybind11::scoped_interpreter guard { true, argv.size(), argv.data() };

    try
    {
        pybind11::eval_file(GRACE_PY_SHADER_COMP_SCRIPT);
    }
    catch (const pybind11::error_already_set& e)
    {
        std::cerr << "Python exception:\n";
        std::cerr << e.what() << std::endl;
        throw;
    }
}

void CompileSlang(std::initializer_list<std::string> shaders,
                  const std::string& spirvDir,
                  const std::string& compiler,
                  std::initializer_list<std::string> compilerArgs)
{
    std::string shadersStr = SingleStringShaders(shaders);
    std::string slangcArgs = SingleStringCompilerArgs(compilerArgs);

    const std::array<const char*, 9> argv = {
        GRACE_PY_SHADER_COMP_SCRIPT,
        "--shaders", shadersStr.c_str(),
        "--spirv-dir", spirvDir.c_str(),
        "--slangc", compiler.c_str(),
        "--slangc-args", slangcArgs.c_str(),
    };

    const pybind11::scoped_interpreter guard { true, argv.size(), argv.data() };

    try
    {
        pybind11::eval_file(GRACE_PY_SHADER_COMP_SCRIPT);
    }
    catch (const pybind11::error_already_set& e)
    {
        std::cerr << "Python exception:\n";
        std::cerr << e.what() << std::endl;
        throw;
    }
}

void CompileGlsl(const std::string& shader,
                 const std::string& spirvDir,
                 const std::string& compiler,
                 std::initializer_list<std::string> compilerArgs)
{
    std::string glslcArgs = SingleStringCompilerArgs(compilerArgs);

    const std::array<const char*, 9> argv = {
        GRACE_PY_SHADER_COMP_SCRIPT,
        "--shaders", shader.c_str(),
        "--spirv-dir", spirvDir.c_str(),
        "--glslc", compiler.c_str(),
        "--glslc-args", glslcArgs.c_str(),
    };

    const pybind11::scoped_interpreter guard { true, argv.size(), argv.data() };

    try
    {
        pybind11::eval_file(GRACE_PY_SHADER_COMP_SCRIPT);
    }
    catch (const pybind11::error_already_set& e)
    {
        std::cerr << "Python exception:\n";
        std::cerr << e.what() << std::endl;
        throw;
    }
}

void CompileGlsl(std::initializer_list<std::string> shaders,
                 const std::string& spirvDir,
                 const std::string& compiler,
                 std::initializer_list<std::string> compilerArgs)
{
    std::string shadersStr = SingleStringShaders(shaders);
    std::string glslcArgs = SingleStringCompilerArgs(compilerArgs);

    const std::array<const char*, 9> argv = {
        GRACE_PY_SHADER_COMP_SCRIPT,
        "--shaders", shadersStr.c_str(),
        "--spirv-dir", spirvDir.c_str(),
        "--glslc", compiler.c_str(),
        "--glslc-args", glslcArgs.c_str(),
    };

    const pybind11::scoped_interpreter guard { true, argv.size(), argv.data() };

    try
    {
        pybind11::eval_file(GRACE_PY_SHADER_COMP_SCRIPT);
    }
    catch (const pybind11::error_already_set& e)
    {
        std::cerr << "Python exception:\n";
        std::cerr << e.what() << std::endl;
        throw;
    }
}

} // namespace Grace::Ext
