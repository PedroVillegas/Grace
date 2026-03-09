import argparse
import os
import subprocess
from timeit import default_timer as timer

SLANG_EXTENSIONS: set[str] = {'.slang'}
HLSL_EXTENSIONS: set[str] = {'.hlsl'}
GLSL_EXTENSIONS: set[str] = {'.glsl', '.vert', '.vs', '.frag', '.fs', '.comp', '.geom', '.gs', '.tesc', '.tese',
                             '.mesh', '.task', '.rgen', '.rmiss', '.rahit', '.rchit', '.rint', '.rcall'}
ALL_SHADER_FILE_EXTENSIONS: set[str] = {'.slang', '.hlsl', '.glsl', '.vert', '.vs', '.frag', '.fs', '.comp', '.geom',
                                        '.gs', '.tesc', '.tese', '.mesh', '.task', '.rgen', '.rmiss', '.rahit',
                                        '.rchit', '.rint', '.rcall'}

PRAGMA_DO_NOT_COMPILE_CHAR_COUNT: int = 23


class ShaderCompilationManager:
    slangc: str | None = None
    slangc_args: str | None = None
    hlslc: str | None = None
    hlslc_args: str | None = None
    glslc: str | None = None
    glslc_args: str | None = None
    shaders: list[str] = []
    spirv_dir: str | None = None

    def __init__(self,
                 shaders: list[str],
                 slangc: str | None,
                 slangc_args: str | None,
                 hlslc: str | None,
                 hlslc_args: str | None,
                 glslc: str | None,
                 glslc_args: str | None,
                 spirv_dir: str | None):

        receivedAtleastOneCompiler = False

        if len(shaders) == 0 or spirv_dir is None:
            print("No shaders received or no SPIR-V output directory received!")
            return

        if slangc is not None:
            self.slangc = slangc
            receivedAtleastOneCompiler = True

        if hlslc is not None:
            self.hlslc = hlslc
            receivedAtleastOneCompiler = True

        if glslc is not None:
            self.glslc = glslc
            receivedAtleastOneCompiler = True

        if not receivedAtleastOneCompiler:
            print("Expected at least one of --slangc, --hlslc, --glslc. Received none!")
            exit()

        self.slangc_args = slangc_args
        self.hlslc_args = hlslc_args
        self.glslc_args = glslc_args
        self.shaders = shaders
        self.spirv_dir = spirv_dir

    def Compile(self):
        for sh in self.shaders:
            begin = timer()
            with open(sh, 'rb') as f:
                if f.read(PRAGMA_DO_NOT_COMPILE_CHAR_COUNT) == b"// GRACE_DO_NOT_COMPILE":
                    return

            filename, extension = os.path.splitext(sh)
            basename = os.path.basename(sh)
            outputfile = f'{self.spirv_dir}/{basename}.spv'

            if not DependenciesModified(outputfile, extension):
                continue

            if extension in SLANG_EXTENSIONS:
                CompileSlang(self.slangc, self.slangc_args, sh, outputfile)
            elif extension in HLSL_EXTENSIONS:
                CompileHlsl(self.hlslc, self.hlslc_args, sh, outputfile)
            elif extension in GLSL_EXTENSIONS:
                CompileGlsl(self.glslc, self.glslc_args, sh, outputfile)
            else:
                print(f"Unknown shader extension -- '{sh}'")

            end = timer()
            print(f"Compiled '{basename}' in {format((end - begin) * 1000.0, '.1f')}ms")


def DependenciesModified(spv: str, ext: str) -> bool:
    depfile = spv + ".d"
    if not os.path.exists(depfile):
        return True

    with open(depfile, 'r') as f:
        # Ignore spv file, colon, space and trailing '\n'
        content = f.read()
        if ext in SLANG_EXTENSIONS:
            content = StripEscapeChar(content)
        deps = content[len(spv) + 2: -1].split()

    paths: list[str] = []
    newPathBeginInd = 0
    for ind, string in enumerate(deps):
        base, ext = os.path.splitext(string)
        path = ""
        if ext in ALL_SHADER_FILE_EXTENSIONS:
            for i in range(newPathBeginInd, ind):
                path += deps[i] + " "
            path += deps[ind]
            paths.append(path)
            newPathBeginInd = ind + 1

    # If input older than output then do not recompile
    for file in paths:
        if os.path.getmtime(file) > os.path.getmtime(spv):
            return True

    return False


def CompileSlang(slangc: str,
                 slangc_args: str | None,
                 filepath: str,
                 outputfile: str):
    cmd: list[str] = [
        f'{slangc}', f'{filepath}',
        '-target', 'spirv',
        '-profile', 'spirv_1_6',
        '-g0',
        '-o', f'{outputfile}',
        '-entry', 'main',
        '-depfile', f'{outputfile}.d'
    ]

    if slangc_args is not None and len(slangc_args) > 0:
        cmd += slangc_args[0].split()

    subprocess.run(cmd)


def CompileHlsl(hlslc: str,
                hlslc_args: str | None,
                filepath: str,
                outputfile: str):
    pass


def CompileGlsl(glslc: str,
                glslc_args: str | None,
                filepath: str,
                outputfile: str):
    cmd: list[str] = [
        f'{glslc}', f'{filepath}',
        '-g',
        '-o', f'{outputfile}',
        '--target-env=vulkan1.3',
        '-MD'
    ]

    if glslc_args is not None and len(glslc_args) > 0:
        cmd += glslc_args[0].split()

    subprocess.run(cmd)

def StripEscapeChar(string: str) -> str:
    stripped = ""
    for i, char in enumerate(string):
        stripped += "" if char == '\\' and string[i-1] != '\\' else char
    return stripped


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Script to compile Slang/HLSL/GLSL shaders to SPIR-V')
    parser.add_argument('--shaders', type=str, nargs='*', help='Path to shaders to compile')
    parser.add_argument('--spirv-dir', type=str, help='Path to SPIR-V output directory')
    parser.add_argument('--slangc', type=str, help='Path to Slang compiler executable')
    parser.add_argument('--slangc-args', type=str, nargs='*', help='Extra Slang compiler arguments')
    parser.add_argument('--hlslc', type=str, help='Path to HLSL compiler executable')
    parser.add_argument('--hlslc-args', type=str, nargs='*', help='Extra HLSL compiler arguments')
    parser.add_argument('--glslc', type=str, help='Path to GLSL compiler executable')
    parser.add_argument('--glslc-args', type=str, nargs='*', help='Extra GLSL compiler arguments')

    args = parser.parse_args()

    scm: ShaderCompilationManager = ShaderCompilationManager(
        args.shaders,
        args.slangc,
        args.slangc_args,
        args.hlslc,
        args.hlslc_args,
        args.glslc,
        args.glslc_args,
        args.spirv_dir
    )

    if not os.path.exists(args.spirv_dir):
        os.mkdir(args.spirv_dir)

    scm.Compile()
