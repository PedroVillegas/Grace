#include <Grace/Ext/ShaderCompiler.hpp>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

void Grace::Ext::CompileShaderSingle(const std::string& relativePath)
{
    const std::string tmp = std::string(GRACE_SHADERS_DIR) + relativePath;
    const char* rp = tmp.c_str();

    // clang-format off
    // The preceding space before the extra compiler args are required to prevent
    // them being interpreted as arguments for the script itself. This seems to only
    // be a problem with just one extra arg specified (no spaces)
    const std::array<const char*, 17> argv = {
        GRACE_PY_SHADER_COMP_SCRIPT,
        "--shaders", rp,
        "--spirv-dir", GRACE_SPIRV_DIR,
        "--slangc", GRACE_SLANG_COMPILER,
        "--slangc-args", " " GRACE_SLANG_COMPILER_ARGS,
        "--hlslc", GRACE_HLSL_COMPILER,
        "--hlslc-args", " " GRACE_HLSL_COMPILER_ARGS,
        "--glslc", GRACE_GLSL_COMPILER,
        "--glslc-args", " " GRACE_GLSL_COMPILER_ARGS,
    };
    // clang-format on

    const pybind11::scoped_interpreter guard { true, argv.size(), argv.data() };

    pybind11::eval_file(GRACE_PY_SHADER_COMP_SCRIPT);
}

void Grace::Ext::CompileShaderMulti(const std::vector<std::string>& relativePaths)
{
    std::vector<const char*> rp(relativePaths.size());

    for (const std::string& path : relativePaths)
    {
        rp.push_back(path.c_str());
    }

    const pybind11::scoped_interpreter guard { true, static_cast<int>(relativePaths.size()), rp.data() };

    pybind11::eval_file(GRACE_PY_SHADER_COMP_SCRIPT);
}
