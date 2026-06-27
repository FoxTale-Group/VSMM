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
![Screenshot of Vintage Story Mod Manager UI](/.images/VSMM.png)

---

## Building

> [!NOTE]
> Currently only building from source is supported. Releases will be added soon.

### Requirements

- A C++23-capable compiler
- [CMake](https://cmake.org/) ≥ 4.3 and [Ninja](https://ninja-build.org/)
- [Qt 6](https://www.qt.io/) — `Core`, `Quick`, `Qml`, `Network`, `QuickControls2`
- [libzip](https://libzip.org/)
- [cpp-semver](https://github.com/easz/cpp-semver) — fetched automatically via CMake `FetchContent`

On Arch Linux:

```bash
sudo pacman -S qt6-base qt6-declarative libzip cmake ninja
```

On Ubuntu:
```bash
sudo apt install cmake ninja-build qt6-{base,declarative}-dev libzip-dev qml6-module-qtquick-controls qml6-module-qtquick-window qml6-module-qtqml-workerscript
```

---

### Building

```bash
cmake -G Ninja -S . -B build-dir -DCMAKE_BUILD_TYPE=Debug && cmake --build build-dir
```

For an optimized build, use `-DCMAKE_BUILD_TYPE=Release` and a matching build dir.
You can name your `build-dir` whatever you like.

### Running

```bash
# Uses the default mods directory (<GenericConfig>/VintagestoryData/Mods)
./build-dir/bin/VSModChecker

# Or point it at a specific mods folder
./build-dir/bin/VSModChecker --mods-dir /path/to/Mods
```

---

## Roadmap

- [x] Scan local mods folder
- [x] Fetch info + update status from the mod API
- [x] Display icons, tags, and update badges
- [x] Search / sort / filter
  - [ ] More filter options
- [x] Add a mod from the GUI
- [ ] Config / Settings
- [ ] Launch Game
- [ ] Check for updates (single + all)
- [ ] Update selected / update all
- [ ] Per-mod actions:
  - [ ] update,
  - [ ] check update,
  - [ ] favorite,
  - [x] open mod's page
  - [ ] delete
- [ ] Enable / disable mods
- [ ] Mod profiles


See the [project board](about:newtab) for the
full list of features, ideas, and bugs.

---

## Contributing

Contributions are welcome! Please open an issue to discuss significant changes
before submitting a pull request. The codebase splits cleanly into a C++ backend
(`src/`) and a QML frontend (`qml/`).

## License

This project is licensed under the **GNU General Public License v3.0** —
see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- The [Vintage Story](https://www.vintagestory.at/) team and modding community
- Built with [Qt](https://www.qt.io/), [libzip](https://libzip.org/),
  and [cpp-semver](https://github.com/easz/cpp-semver)
