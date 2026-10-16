#!/usr/bin/env python3
r"""VSMM build helper for Linux and Windows.

    ./scripts/build.py              # Release build into build/
    ./scripts/build.py -d           # Debug build into build-debug/
    ./scripts/build.py -d -r        # Debug build, then run it
    ./scripts/build.py -t           # Debug build, then run the tests
    ./scripts/build.py -b out -r    # Release build into out/, then run
    ./scripts/build.py -d -- -DENABLE_COVERAGE=ON
                                    # extra arguments for the configure step

Runnable from the project root, from scripts/, or from anywhere else.

On Windows, run it as `py scripts\build.py`. The Visual Studio C++ environment is loaded
automatically when cl.exe is not already on PATH, and vcpkg is taken from VCPKG_ROOT. CMake
needs to find Qt once per build directory: set the CMAKE_PREFIX_PATH environment variable,
or pass it on the first run:

    py scripts\build.py -- -DCMAKE_PREFIX_PATH=C:\Qt\6.10.0\msvc2022_64
"""

# The module docstring above doubles as the --help text, so keep commentary out of it.

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

IS_WINDOWS = sys.platform == "win32"

APP_BINARY_NAME = "VSMM.exe" if IS_WINDOWS else "VSMM"

REPO_ROOT = Path(__file__).resolve().parent.parent


class BuildError(Exception):
    pass


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "-d", "--debug",
        action="store_true",
        help="Debug build (default: Release)",
    )
    parser.add_argument(
        "-b", "--build-dir",
        type=Path,
        default=None,
        metavar="DIR",
        help="Build directory, relative to the current directory "
             "(default: build, or build-debug with --debug, in the project root)",
    )
    parser.add_argument(
        "-t", "--test",
        action="store_true",
        help="Run the tests after a successful build (implies --debug; Release "
             "builds do not compile the tests)",
    )
    parser.add_argument(
        "-r", "--run",
        action="store_true",
        help="Run the app after a successful build (and tests, with --test)",
    )
    parser.add_argument(
        "cmake_args",
        nargs="*",
        metavar="CMAKE_ARG",
        help="Extra arguments for the configure step, given after --",
    )
    args = parser.parse_args()
    # A bare word would otherwise reach CMake, which reads it as a source or build path.
    if args.cmake_args and "--" not in sys.argv[1:]:
        parser.error("extra CMake arguments must come after --")
    if args.test:
        args.debug = True
    return args


def log(message: str) -> None:
    # Flush so our lines stay in order with the child processes' output when stdout is a pipe.
    print(f"==> {message}", flush=True)


def run(command: list[str], env: dict[str, str]) -> None:
    """Run a command, raising CalledProcessError if it fails."""
    log(" ".join([Path(command[0]).name, *command[1:]]))
    subprocess.run(command, env=env, check=True)


def find_tool(name: str, env: dict[str, str]) -> str:
    # Resolve against the child's PATH: on Windows, CreateProcess searches the parent's PATH,
    # so a tool that only the loaded MSVC environment provides would not be found otherwise.
    path = shutil.which(name, path=env.get("PATH"))
    if path is None:
        raise BuildError(f"{name} not found on PATH")
    return path


def parse_env_block(text: str) -> dict[str, str]:
    """Parse the output of cmd's `set` into an environment mapping."""
    env = {}
    for line in text.splitlines():
        key, sep, value = line.partition("=")
        # cmd also prints hidden per-drive entries like "=C:=C:\...", which have no name.
        if sep and key:
            # Upper-cased to match os.environ on Windows; cmd prints "Path", not "PATH".
            env[key.upper()] = value
    return env


def msvc_env() -> dict[str, str]:
    """Return an environment with the MSVC x64 toolchain on PATH."""
    if shutil.which("cl"):
        return dict(os.environ)

    program_files = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = Path(program_files) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.is_file():
        raise BuildError(
            "Visual Studio not found; install it (or the Build Tools) with the "
            "'Desktop development with C++' workload"
        )

    install_dir = subprocess.run(
        [
            str(vswhere), "-latest", "-products", "*",
            "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "-property", "installationPath",
        ],
        capture_output=True, text=True, check=True,
    ).stdout.strip()
    if not install_dir:
        raise BuildError("no Visual Studio installation with the C++ x64 build tools found")

    vcvars = Path(install_dir) / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
    log(f"loading MSVC environment from {vcvars}")
    output = subprocess.run(
        f'"{vcvars}" >nul && set',
        shell=True, capture_output=True, text=True, errors="replace", check=True,
    ).stdout
    env = parse_env_block(output)
    find_tool("cl", env)
    return env


def cache_value(build_dir: Path, name: str) -> str | None:
    """Return a variable from the build directory's CMakeCache.txt, if it is set."""
    try:
        lines = (build_dir / "CMakeCache.txt").read_text(errors="replace").splitlines()
    except FileNotFoundError:
        return None
    prefix = f"{name}:"
    for line in lines:
        if line.startswith(prefix):
            return line.partition("=")[2]
    return None


def with_runtime_path(env: dict[str, str], build_dir: Path) -> dict[str, str]:
    """Put the project's and Qt's DLL directories on PATH.

    Windows has no rpath, and each project library is built into its own directory, so the
    app and the tests can only find their DLLs through PATH.
    """
    # vcpkg_installed holds Release and Debug builds of the same DLL names; the ones each
    # target needs are already copied next to it at build time.
    dirs = sorted({
        str(dll.parent)
        for dll in build_dir.rglob("*.dll")
        if "vcpkg_installed" not in dll.parts
    })

    qt_cmake_dir = cache_value(build_dir, "Qt6_DIR")
    if qt_cmake_dir:
        qt_bin = Path(qt_cmake_dir).parents[2] / "bin"  # Qt6_DIR is <prefix>/lib/cmake/Qt6
        if qt_bin.is_dir():
            dirs.append(str(qt_bin))

    return {**env, "PATH": os.pathsep.join([*dirs, env.get("PATH", "")])}


def main() -> int:
    args = parse_args()

    if not (REPO_ROOT / "CMakeLists.txt").is_file():
        raise BuildError(f"cannot find CMakeLists.txt in {REPO_ROOT}")

    build_type = "Debug" if args.debug else "Release"

    # Separate default directories so Debug and Release do not share one cache.
    if args.build_dir is not None:
        build_dir = args.build_dir.resolve()
    else:
        build_dir = REPO_ROOT / ("build-debug" if args.debug else "build")

    env = msvc_env() if IS_WINDOWS else dict(os.environ)
    cmake = find_tool("cmake", env)
    find_tool("ninja", env)
    ctest = find_tool("ctest", env) if args.test else None

    log(f"{build_type} build in {build_dir}")

    configure = [
        cmake,
        "-G", "Ninja",
        "-S", str(REPO_ROOT),
        "-B", str(build_dir),
        f"-DCMAKE_BUILD_TYPE={build_type}",
    ]
    if IS_WINDOWS:
        # Without these, CMake may pick up another compiler on PATH (MinGW, Clang).
        configure += ["-DCMAKE_C_COMPILER=cl", "-DCMAKE_CXX_COMPILER=cl"]
        vcpkg_root = env.get("VCPKG_ROOT") or env.get("VCPKG_INSTALLATION_ROOT")
        if vcpkg_root:
            toolchain = Path(vcpkg_root) / "scripts" / "buildsystems" / "vcpkg.cmake"
            configure.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain}")
    # Last, so a -D given here overrides any default above.
    configure += args.cmake_args

    run(configure, env)
    run([cmake, "--build", str(build_dir)], env)

    if IS_WINDOWS and (args.test or args.run):
        env = with_runtime_path(env, build_dir)

    if ctest is not None:
        run([ctest, "--test-dir", str(build_dir), "--output-on-failure", "--no-tests=error"], env)

    if args.run:
        app_path = build_dir / "bin" / APP_BINARY_NAME
        if not app_path.is_file() or not os.access(app_path, os.X_OK):
            raise BuildError(f"{app_path} not found or not executable")

        log(f"running {app_path}")
        if IS_WINDOWS:
            # Windows has no real exec: os.execv starts a new process and exits this one,
            # so the shell prompt returns while the app runs and its exit code is lost.
            return subprocess.run([str(app_path)], env=env).returncode
        # Replace this process so Ctrl+C and the app's exit code reach the caller directly.
        os.execv(app_path, [str(app_path)])

    log("done")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BuildError as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)
    except subprocess.CalledProcessError as exc:
        # The failing tool already printed its errors; a traceback would only bury them.
        print(f"error: command failed with exit code {exc.returncode}", file=sys.stderr)
        sys.exit(exc.returncode)
    except KeyboardInterrupt:
        print("\ninterrupted", file=sys.stderr)
        sys.exit(130)
