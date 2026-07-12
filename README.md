![Project State](https://img.shields.io/badge/Status-Early_Dev-orange)
![Stable Release](https://img.shields.io/github/v/release/FoxTale-Group/VSMM?logo=github&label=Stable&color=blue)
![Pre-Release](https://img.shields.io/github/v/release/FoxTale-Group/VSMM?include_prereleases&logo=github&label=Pre-Release&color=orange)
![Windows Build](https://github.com/FoxTale-Group/VSMM/actions/workflows/build-windows.yml/badge.svg?branch=next)
![Linux Build](https://github.com/FoxTale-Group/VSMM/actions/workflows/build.yml/badge.svg?branch=next)

# Vintage Story Mod Manager (VSMM)

A fast, lightweight desktop manager for [*Vintage Story*](https://www.vintagestory.at/) mods.

VSMM scans your local mods folder, cross-references each installed mod against the
official [Vintage Story mod API](https://mods.vintagestory.at/), and shows you at a
glance which mods have updates available — all inside a custom, frameless, dark-themed UI.

## Features

- **Automatic scanning** of your local `Mods` folder
- **Update detection** - compares installed versions against the latest release
  via semantic versioning
- **Rich mod list** with icons, authors, tags, and update badges
- **Updating mods** - with option to update all/selected/single mod.
- **Instant search, sort, and filter**
- **Add mods from the GUI**
- **Custom frameless dark UI** built with Qt Quick / QML

> [!WARNING]
> Vintage Story Mod Manager is currently Under Development. Some things may not work, some may not be implemented yet.
> Keep that in mind when using unfinished versions of our app.

## Screenshots

<!-- Add screenshots or a GIF here -->
![Screenshot of Vintage Story Mod Manager UI](/.docs/images/VSMM.png)

---

## Releases & Running

[Latest release](https://github.com/FoxTale-Group/VSMM/releases/latest)

### Runtime Dependencies
- Qt 6.10
- libzip

---

## Building from source

### Requirements

- A C++23-capable compiler
- [CMake](https://cmake.org/) ≥ 4.2 and [Ninja](https://ninja-build.org/)
- [Qt 6](https://www.qt.io/) - `Core`, `Quick`, `Qml`, `Network`, `QuickControls2`
- [libzip](https://libzip.org/)


Install required build dependencies using your distribution package manager.
```bash
# For Arch:
sudo pacman -S cmake ninja qt6-base qt6-declarative libzip 

# For Ubuntu:
sudo apt install cmake ninja-build qt6-{base,declarative}-dev libzip-dev

# For Fedora:
sudo dnf install cmake ninja-build libzip-devel qt6-qtbase-devel qt6-qtquickcontrols2-devel
```

### Building

```bash
# Clone this repository:
git clone https://github.com/FoxTale-Group/VSMM.git
cd VSMM

# Build:
cmake -G Ninja -S . -B build-dir -DCMAKE_BUILD_TYPE=Release && cmake --build build-dir
```
> [!NOTE]
> For an optimized build, use `-DCMAKE_BUILD_TYPE=Release` and a matching build directory.
> Use `-DCMAKE_BUILD_TYPE=Debug` for more detailed verbose logs.
> You can name your `build-dir` whatever you like.

> [!TIP]
> You can also use `build.sh` or `build-debug.sh` scripts.


### Running

```bash
./build-dir/bin/VSMM
```

> [!IMPORTANT]
> After starting go to app settings and setup path to `VintagestoryData` folder.

---

## Roadmap

**1.0 Release:**
- [x] Scan local mods folder
- [x] Fetch info + update status from the mod API
- [x] Display icons, tags, and update badges
- [x] Search / sort
- [x] Adding new mods from the GUI
- [x] Update mods (https://github.com/FoxTale-Group/VSMM/issues/19)
- [x] Config / Settings (https://github.com/FoxTale-Group/VSMM/issues/16)
- [x] Launching Game (https://github.com/FoxTale-Group/VSMM/issues/20)
- [x] Per-mod actions (https://github.com/FoxTale-Group/VSMM/issues/26)
- [ ] More filter/search options (https://github.com/FoxTale-Group/VSMM/issues/15)

**1.1 Release:**
- [ ] Enable / disable mods (https://github.com/FoxTale-Group/VSMM/issues/23)
- [ ] Mod profiles (https://github.com/FoxTale-Group/VSMM/issues/21)


See the [project board](https://github.com/orgs/FoxTale-Group/projects/2/views/1) for the
full list of features, ideas, and bugs.

---

## Contributing

Contributions are welcome! Check [CONTRIBUTING.md](CONTRIBUTING.md)

## License

This project is licensed under the **GNU General Public License v3.0** —
see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- The [Vintage Story](https://www.vintagestory.at/) team and modding community
- Built with [Qt](https://www.qt.io/), [libzip](https://libzip.org/),
  and [cpp-semver](https://github.com/easz/cpp-semver)
