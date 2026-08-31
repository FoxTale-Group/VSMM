#!/usr/bin/env python3

# Build a VSMM AppImage from an existing CMake build directory.
#
#     ./scripts/build_appimage.py                      uses build/
#     ./scripts/build_appimage.py build-debug          uses build-debug/
#
# linuxdeploy and linuxdeploy-plugin-qt are downloaded on demand into a cache directory, so
# no separate setup step is needed on a CI runner. Runnable from anywhere.


import argparse
import os
import shutil
import re
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

# Release tag of the linuxdeploy tools to fetch. "continuous" tracks their latest build;
# override with VSMM_LINUXDEPLOY_TAG to pin a specific release for reproducible packaging.
TOOL_TAG = os.environ.get("VSMM_LINUXDEPLOY_TAG", "continuous")

# Tool name as invoked -> (GitHub repo, asset filename).
TOOLS = {
    "linuxdeploy": ("linuxdeploy/linuxdeploy", "linuxdeploy-x86_64.AppImage"),
    "linuxdeploy-plugin-qt": (
        "linuxdeploy/linuxdeploy-plugin-qt",
        "linuxdeploy-plugin-qt-x86_64.AppImage",
    ),
}

QMAKE_CANDIDATES = (
    Path("/usr/lib/qt6/bin/qmake6"),
    Path("/usr/lib/qt6/bin/qmake"),
    Path("/usr/bin/qmake6"),
    Path("/usr/bin/qmake"),
)

DOWNLOAD_TIMEOUT_SECONDS = 120


def fail(message: str) -> None:
    print(f"error: {message}", file=sys.stderr)
    sys.exit(1)


def download(url: str, destination: Path) -> None:
    """Fetch url to destination, atomically and marked executable."""
    print(f"==> downloading {url}")
    # Download to a temporary name and rename on success, so an interrupted or failed
    # download can never leave a truncated binary that looks usable on the next run.
    partial = destination.with_suffix(destination.suffix + ".partial")
    try:
        with urllib.request.urlopen(url, timeout=DOWNLOAD_TIMEOUT_SECONDS) as response:
            with partial.open("wb") as handle:
                shutil.copyfileobj(response, handle)
    except (urllib.error.URLError, TimeoutError) as exc:
        partial.unlink(missing_ok=True)
        fail(f"could not download {url}: {exc}")

    # An HTTP error page or a redirect to HTML would otherwise be saved and later fail with
    # a confusing "Exec format error". ELF files start with 0x7F 'E' 'L' 'F'.
    with partial.open("rb") as handle:
        if handle.read(4) != b"\x7fELF":
            partial.unlink(missing_ok=True)
            fail(f"{url} did not return an executable")

    partial.replace(destination)
    destination.chmod(0o755)


def ensure_tools(tools_dir: Path) -> None:
    """Put linuxdeploy and its Qt plugin on PATH, downloading them if absent."""
    # Anything already on PATH wins, so a system package or a CI-provided copy is respected.
    if all(shutil.which(name) is not None for name in TOOLS):
        return

    tools_dir.mkdir(parents=True, exist_ok=True)

    for name, (repo, asset) in TOOLS.items():
        # linuxdeploy locates its plugins by name on PATH, so each tool needs a plain-named
        # entry point rather than the versioned asset filename.
        entry_point = tools_dir / name
        if entry_point.exists():
            continue
        download(
            f"https://github.com/{repo}/releases/download/{TOOL_TAG}/{asset}",
            entry_point,
        )

    # Prepend so these copies win over anything else that appears later on PATH.
    os.environ["PATH"] = f"{tools_dir}{os.pathsep}{os.environ['PATH']}"

    missing = [name for name in TOOLS if shutil.which(name) is None]
    if missing:
        fail(f"tools still not runnable after download: {', '.join(missing)}")


def read_version() -> str:
    """Return VSMM_RELEASE_VERSION from CMakeLists.txt, the single source of truth."""
    cmakelists = REPO_ROOT / "CMakeLists.txt"
    match = re.search(
        r'^set\(VSMM_RELEASE_VERSION "([^"]*)"',
        cmakelists.read_text(encoding="utf-8"),
        re.MULTILINE,
    )
    if match is None or not match.group(1):
        fail(f"could not read VSMM_RELEASE_VERSION from {cmakelists}")
    return match.group(1)


def find_qmake() -> Path:
    """Return a Qt6 qmake.

    linuxdeploy-plugin-qt locates Qt through qmake, and distributions that ship Qt5 and Qt6
    side by side usually put Qt5's first on PATH. The plugin will bundle Qt5 against a Qt6
    binary without complaint, producing an AppImage that aborts at startup.
    """
    override = os.environ.get("QMAKE")
    if override:
        return Path(override)

    for candidate in QMAKE_CANDIDATES:
        if not (candidate.is_file() and os.access(candidate, os.X_OK)):
            continue
        probe = subprocess.run(
            [str(candidate), "-query", "QT_VERSION"],
            capture_output=True,
            text=True,
            check=False,
        )
        if probe.returncode != 0:
            continue
        version = probe.stdout.strip()
        if version.split(".")[0] == "6":
            print(f"==> using Qt {version} from {candidate}")
            return candidate

    fail("no Qt6 qmake found; set QMAKE=/path/to/qmake6")
    raise AssertionError("unreachable")


def run(command: list[str], cwd: Path) -> None:
    """Run a command in cwd, raising CalledProcessError on failure."""
    print(f"==> {' '.join(command)}")
    subprocess.run(command, cwd=cwd, check=True)


def deploy_environment() -> dict[str, str]:
    """Environment variables that configure linuxdeploy-plugin-qt."""
    return {
        # VSMM's QML is compiled into the binary as qrc, so there is no QML on disk for
        # qmlimportscanner to read. It scans these source paths instead. DeployHints.qml
        # names modules reached from C++ rather than from a QML import.
        #
        # "Missing qml module: vsmm / VSMMStyle" in the output is expected and harmless:
        # both live in qrc inside the binary.
        "QML_SOURCES_PATHS": os.pathsep.join(
            [
                str(REPO_ROOT / "src" / "qml"),
                str(REPO_ROOT / "packaging" / "appimage"),
            ]
        ),
        # Qt loads the SVG image plugin by name at runtime, so nothing links libQt6Svg and
        # no scanner can see it. Without this the AppImage runs but renders no icons.
        "EXTRA_QT_MODULES": "svg",
        # Only libqxcb is deployed by default. offscreen is what lets CI smoke-test the
        # finished AppImage on a runner with no display.
        "EXTRA_PLATFORM_PLUGINS": "libqoffscreen.so",
        # linuxdeploy bundles a binutils strip that cannot parse .relr.dyn sections emitted
        # by current toolchains, and a strip failure aborts the whole run. Distro Qt
        # libraries are already stripped, so little size is lost.
        "NO_STRIP": os.environ.get("NO_STRIP", "1"),
        # The tools are AppImages and normally mount themselves via FUSE, which CI runners
        # usually lack. This makes them unpack to a temporary directory instead.
        "APPIMAGE_EXTRACT_AND_RUN": "1",
    }


def prune_orphaned_libs(appdir: Path) -> None:
    """Remove usr/lib/vsmm once linuxdeploy has relocated its contents to usr/lib.

    cmake installs VSMM's libraries to a private subdirectory; linuxdeploy copies them to
    usr/lib and rewrites RUNPATH to point there, leaving the originals as dead weight.
    """
    private_dir = appdir / "usr" / "lib" / "vsmm"
    if not private_dir.is_dir():
        return

    orphans = list(private_dir.glob("*.so"))
    # Only delete once every library is confirmed present in the new location, so a change
    # in linuxdeploy's behaviour cannot remove the only copy.
    if orphans and all((appdir / "usr" / "lib" / lib.name).exists() for lib in orphans):
        print(f"==> pruning {len(orphans)} superseded copies in usr/lib/vsmm")
        shutil.rmtree(private_dir)
    else:
        print("==> keeping usr/lib/vsmm: not every library was relocated", file=sys.stderr)


USAGE_EXAMPLES = """
examples:
  ./scripts/build_appimage.py                  package build/
  ./scripts/build_appimage.py build-debug      package build-debug/

The linuxdeploy tools are downloaded on demand, so no setup step is needed.
Set VSMM_LINUXDEPLOY_TAG to pin their release for reproducible packaging.
"""


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build a VSMM AppImage from an existing CMake build directory.",
        # Keeps the example block's line breaks; the default formatter reflows it.
        epilog=USAGE_EXAMPLES,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "build_dir",
        nargs="?",
        default=Path("build"),
        type=Path,
        metavar="BUILD_DIR",
        help="CMake build directory to package (default: build)",
    )
    parser.add_argument(
        "--tools-dir",
        type=Path,
        default=Path(
            os.environ.get(
                "VSMM_APPIMAGE_TOOLS", Path.home() / ".cache" / "vsmm-appimage-tools"
            )
        ),
        help="Where to cache the linuxdeploy tools",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    build_dir: Path = args.build_dir

    if not build_dir.is_dir():
        fail(f"build directory '{build_dir}' does not exist; configure and build first")

    appdir = build_dir / "AppDir"

    ensure_tools(args.tools_dir)

    version = read_version()
    print(f"==> building AppImage for VSMM {version}")

    # linuxdeploy reads VERSION when naming its output.
    os.environ["VERSION"] = version
    os.environ.update(deploy_environment())
    os.environ["QMAKE"] = str(find_qmake())

    # linuxdeploy only ever adds files, so a stale AppDir would ship a library that has
    # since been removed from the project.
    shutil.rmtree(appdir, ignore_errors=True)
    run(
        ["cmake", "--install", str(build_dir), "--prefix", str(appdir / "usr")],
        cwd=REPO_ROOT,
    )

    # Three invocations rather than `linuxdeploy --plugin qt --output appimage`, because
    # --exclude-library is only accepted by the Qt plugin when it is invoked directly.
    run(["linuxdeploy", "--appdir", "AppDir"], cwd=build_dir)

    # kimg_*: KDE's kimageformats installs extra decoders (RAW, EXR, JPEG-XR) into Qt's
    # plugin directory on some distributions. VSMM only loads PNG and SVG, and one of those
    # decoders with an unresolved dependency aborts the entire deployment.
    run(
        ["linuxdeploy-plugin-qt", "--appdir", "AppDir", "--exclude-library", "kimg_*"],
        cwd=build_dir,
    )

    prune_orphaned_libs(appdir)

    run(["linuxdeploy", "--appdir", "AppDir", "--output", "appimage"], cwd=build_dir)

    images = sorted(build_dir.glob("*.AppImage"))
    if not images:
        fail("linuxdeploy reported success but produced no .AppImage")
    for image in images:
        print(f"==> {image}")

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as exc:
        # The failing command already printed its own diagnostics; a traceback would only
        # bury them.
        print(f"error: {exc.cmd[0]} failed with exit code {exc.returncode}", file=sys.stderr)
        sys.exit(exc.returncode)
    except KeyboardInterrupt:
        print("\ninterrupted", file=sys.stderr)
        sys.exit(130)
