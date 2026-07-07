![Static Badge](https://img.shields.io/badge/Status-Early_Dev-orange?style=for-the-badge)
![Static Badge](https://img.shields.io/badge/Backend-C%2B%2B23-blue?style=for-the-badge&logo=cplusplus)
![Static Badge](https://img.shields.io/badge/GUI-Qt%206-dark_green?style=for-the-badge&logo=qt&logoColor=white)
![Static Badge](https://img.shields.io/badge/CMake-4.3%2B-red?style=for-the-badge&logo=cmake)

# Vintage Story Mod Manager (VSMM)

A fast, lightweight desktop manager for [*Vintage Story*](https://www.vintagestory.at/) mods.

VSMM scans your local mods folder, cross-references each installed mod against the
official [Vintage Story mod API](https://mods.vintagestory.at/), and shows you at a
glance which mods have updates available — all inside a custom, frameless, dark-themed UI.

> **Status:** Early Development.
> 
> Core scanning, update-checking, and the mod list are working; several actions are still being built (see [Roadmap](#roadmap)).

---

## Features

- **Automatic scanning** of your local `Mods` folder
- **Update detection** — compares installed versions against the latest release
  via semantic versioning
- **Rich mod list** with icons, authors, tags, and update badges
- **Instant search, sort, and filter**
- **Add mods from the GUI**
- **Custom frameless dark UI** built with Qt Quick / QML

## Screenshots

<!-- Add screenshots or a GIF here -->
![Screenshot of Vintage Story Mod Manager UI](/.docs/images/VSMM.png)

---

## Releases & Running

[Latest release](https://github.com/FoxTale-Group/VSMM/releases/latest)

### Runtime Dependencies
- Qt 6.11
- libzip

(this section is work in progress)

---

## Building from source

### Requirements

- A C++23-capable compiler
- [CMake](https://cmake.org/) ≥ 4.3 and [Ninja](https://ninja-build.org/)
- [Qt 6](https://www.qt.io/) - `Core`, `Quick`, `Qml`, `Network`, `QuickControls2`
- [libzip](https://libzip.org/)
- [cpp-semver](https://github.com/easz/cpp-semver) - fetched automatically via CMake `FetchContent`


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
# Clone this repository from `main` branch:
git clone https://github.com/FoxTale-Group/VSMM.git
cd VSMM

# Build:
cmake -G Ninja -S . -B build-dir -DCMAKE_BUILD_TYPE=Release && cmake --build build-dir
```
> [!NOTE]
> For an optimized build, use `-DCMAKE_BUILD_TYPE=Release` and a matching build directory.
> Use `-DCMAKE_BUILD_TYPE=Release` for more detailed verbose logs.
> You can name your `build-dir` whatever you like.

> [!TIP]
> You can also use `build.sh` or `build-debug.sh` scripts.


### Running

```bash
# Uses the default mods directory (~/.config/VintagestoryData/Mods)
./build-dir/bin/VSModChecker

# Or point it at a specific mods folder
./build-dir/bin/VSModChecker --mods-dir /path/to/Mods
```

---

## Roadmap

- [x] Scan local mods folder
- [x] Fetch info + update status from the mod API
- [x] Display icons, tags, and update badges
- [x] Search / sort
- [x] Add a mod from the GUI
- [x] Check for updates
- [ ] More filter/search options (https://github.com/FoxTale-Group/VSMM/issues/15)
- [ ] Update mods (https://github.com/FoxTale-Group/VSMM/issues/19)
- [ ] Config / Settings (https://github.com/FoxTale-Group/VSMM/issues/16)
- [ ] Launch Game (https://github.com/FoxTale-Group/VSMM/issues/20)
- [ ] Per-mod actions (https://github.com/FoxTale-Group/VSMM/issues/26)
- [ ] Enable / disable mods (https://github.com/FoxTale-Group/VSMM/issues/23)
- [ ] Mod profiles (https://github.com/FoxTale-Group/VSMM/issues/21)


See the [project board](https://github.com/orgs/FoxTale-Group/projects/2/views/1) for the
full list of features, ideas, and bugs.

---

## Contributing

Contributions are welcome! Please open an issue to discuss significant changes
before submitting a pull request. The codebase splits cleanly into a C++ backend
(`src/`) and a QML frontend (`src/qml/`).

## License

This project is licensed under the **GNU General Public License v3.0** —
see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- The [Vintage Story](https://www.vintagestory.at/) team and modding community
- Built with [Qt](https://www.qt.io/), [libzip](https://libzip.org/),
  and [cpp-semver](https://github.com/easz/cpp-semver)
