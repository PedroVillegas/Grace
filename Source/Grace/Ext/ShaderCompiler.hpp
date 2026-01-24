#pragma once

#include <Grace/GraceExport.h>
#include <vector>
#include <string>

namespace Grace::Ext
{

GRACE_EXPORT void CompileShaderSingle(const std::string& relativePath);

GRACE_EXPORT void CompileShaderMulti(const std::vector<std::string>& relativePaths);

} // namespace Grace::Ext
