# Backend API exposed to QML

Everything the C++ backend makes available to the QML layer: singletons, their properties,
invokable methods, model roles, the image provider, and context properties.

Compiled from the headers on 2026-08-13, revised 2026-08-30 after the mod-add refactor. Each entry cites its source file so it can be
verified and kept honest — if something here disagrees with the code, the code wins.

---

## How these reach QML

All six backend types are registered with `QML_SINGLETON` and live in **their own QML
modules**, one per library (`Config`, `ModStore`, `ModListModel`, `ModLoader`,
`ModSortFilterModel`, `GameMngr`).

The app's own module declares them as dependencies in `src/CMakeLists.txt`:

```cmake
qt_add_qml_module(${PROJECT_NAME}
        URI vsmm
        ...
        IMPORTS
            Config ModStore ModListModel ModLoader ModSortFilterModel GameMngr)
```

Because of that, **`import vsmm` is all a QML file needs** — the singletons are reachable by
name with no extra import:

```qml
import vsmm

Label { text: ModStore.installedModsCount }
```

Instances are created by the engine and wired together in `App::initQmlEngine()`
(`src/App.cpp:56-109`).

### Overview

| Singleton | Module | Properties | Invokables | Use it for |
|---|---|---|---|---|
| `Config` | `Config` | 3 | 1 | reading / writing user settings |
| `ModStore` | `ModStore` | 4 | 7 | mod counts, selection state, all mutations except adding |
| `ModSortFilterModel` | `ModSortFilterModel` | 1 | — | the model to feed a `ListView` |
| `ModListModel` | `ModListModel` | — | — | source model; defines the delegate roles |
| `GameMngr` | `GameMngr` | 1 | 1 | launching the game, installed game version |
| `ModLoader` | `ModLoader` | — | 1 | installing a mod from a local file |

---

## Config

`src/interfaces/IConfig.hpp:30-32` · implemented by `src/config/Config.hpp:30-50` · persisted
to `config.json` in `AppConfigLocation`

Three read/write properties, each a **typed `Q_GADGET` value type** declared in
`src/interfaces/ConfigTypes.hpp`, not a string→variant map.

> The properties are declared on the abstract `IConfig` base, not on `Config` itself, so
> grepping `Config.hpp` for them finds nothing. QML is unaffected: it sees them on the
> `Config` singleton through the metaobject as usual. `IConfig` is not a QML type.

| Property | Type | Access | Notify |
|---|---|---|---|
| `general` | `GeneralSettings` | read / write | `generalChanged` |
| `paths` | `PathSettings` | read / write | `pathsChanged` |
| `appearance` | `AppearanceSettings` | read / write | `appearanceChanged` |

### Members of each section

| Section | Member | Type in QML | Default | Meaning |
|---|---|---|---|---|
| `general` | `deleteOldModVersion` | bool | `true` | send the superseded zip to the trash when a mod is **updated** — a downloaded update, or a manual install over a mod that is already known. A scan never deletes, whatever this says |
| `general` | `includeModPrerelease` | bool | `false` | consider pre-release versions when checking for updates |
| `paths` | `gameConfig` | string | `""` | the `VintagestoryData` folder |
| `paths` | `gameExe` | string | `""` | the Vintage Story executable |
| `appearance` | `theme` | int | `0` (`Dark`) | `vsmm::appearance::Theme`, `0` = Dark, `1` = Light |
| `appearance` | `accentIndex` | int | `0` | index of the selected accent colour |

Members and defaults come from `ConfigTypes.hpp:51-147`. Unlike the old hashes, **every
member always exists**, so no `??` guard is needed on a read.

`theme` reaches QML as a plain number: the enum lives in a `Q_NAMESPACE` that is not
registered as a QML type, so `Theme.Light` cannot be named from QML, only `1`.

### Reading

```qml
Label { text: Config.paths.gameConfig }
```

### Writing — assign the member, then save

Value-type write-back means a member assignment runs the whole section setter, so it works
directly, no copy-and-assign-back dance:

```qml
Config.paths.gameConfig = "/new/path"   // runs Config::setPaths, fires pathsChanged
Config.saveToFile()                     // required, the write alone does not persist
```

Three consequences of going through the setter each time:

- **One notify per assignment**, not one per batch. Setting `gameConfig` and then `gameExe`
  emits `pathsChanged` twice.
- **Assigning an unchanged value is a no-op** and emits nothing, so a save-everything handler
  costs only the signals that really changed.
- Only the matching path signal follows: writing `gameExe` fires `gameExePathChanged`, writing
  `gameConfig` fires `gameConfigPathChanged` (`Config.cpp:95-103`).

`Settings.qml:112-125` (`saveToConfig()`) is the working reference.

### Invokable methods

| Method | Signature | Effect |
|---|---|---|
| `saveToFile()` | — | writes all three sections plus favorites to `config.json` |

Nothing else persists on its own. The destructor also saves (`Config.cpp:69`), which is why
values survive a clean shutdown without an explicit call.

### Not exposed to QML

`getFavorites()`, `setFavorites()` and `validate()` are plain C++ members with no
`Q_INVOKABLE` — they exist for the backend only.

---

## ModStore

`src/modstore/ModStore.hpp:30-102` · the entry point for everything that changes mods **except adding one**, which goes to `ModLoader.load()`

### Properties — all read-only

| Property | Type | Notify | Meaning |
|---|---|---|---|
| `installedModsCount` | int | `modsChanged` | number of installed mods |
| `updatesCount` | int | `modsChanged` | how many have an update available |
| `workPending` | bool | `workChanged` | an update or a reload is in progress |
| `modsSelected` | bool | `modSelected` | at least one mod is checked for update |

`installedModsCount` and `updatesCount` **share one notify signal**, so both re-evaluate
whenever either changes. Harmless, but worth knowing if you ever profile bindings.

`workPending` starts as `true` (`ModStore.hpp:92`) — the store considers itself busy until
the first reload finishes, so anything gated on it is disabled at startup by design.

### Invokable methods

| Method | Signature | Effect |
|---|---|---|
| `reload()` | — | re-scan the mods directories |
| `update(id)` | `string` | update one mod |
| `updateAll()` | — | update every mod with an available update |
| `updateSelected()` | — | update the mods checked in the list |
| `markForUpdate(id, marked)` | `string, bool` | check / uncheck a mod; drives `modsSelected` |
| `setFavorite(id, favorite)` | `string, bool` | toggle favorite; persisted via Config |
| `remove(id)` | `string` | uninstall a mod |

`id` is always the `modId` role from the list model.

```qml
Button {
    enabled: !ModStore.workPending && ModStore.updatesCount > 0
    onClicked: ModStore.updateAll()
}
```

### Signals

Only the four notify signals above are meant for QML bindings. The rest — `modAdded`,
`modUpdated`, `modsReloading`, `modUpdateRequested`, `modRemoved` — are internal wiring
between the backend libraries (each is commented as such in the header). They are technically
reachable from QML, but treat them as private.

---

## ModSortFilterModel

`src/modsortfiltermodel/ModSortFilterModel.hpp:25-48` · a `QSortFilterProxyModel` over `ModListModel`

| Property | Type | Access | Notify |
|---|---|---|---|
| `filterText` | string | read / write | `filterTextChanged` |

**This is the model a view should use** — never `ModListModel` directly:

```qml
ListView {
    model: ModSortFilterModel
    delegate: ModEntry {}
}

TextField {
    onTextChanged: ModSortFilterModel.filterText = text
}
```

Two behaviours worth knowing, both from `ModSortFilterModel.cpp`:

- **Filtering matches `modName` only** — case-insensitive `contains`. Author, tags and id are
  not searched.
- **Sorting is by `modName`**, case-insensitive ascending. There is no exposed way to change
  the sort key or direction from QML.

---

## ModListModel

`src/modlistmodel/ModListModel.hpp:29-68` · the source model — bind views to
`ModSortFilterModel` instead

No properties or invokables. Its contribution is **the role names available inside a
delegate**, which are the contract every mod-row component is written against.

| Role | Type in QML | Source |
|---|---|---|
| `modId` | string | `ModEntry::getId()` |
| `modName` | string | `getName()` |
| `modAuthor` | string | `getAuthor()` |
| `modVersion` | string | installed version, semver rendered to string |
| `modLatestVersion` | string | latest known version, semver rendered to string |
| `modHasUpdate` | bool | `hasUpdate()` |
| `modTags` | list of string | `getTags()` (`QStringList`) |
| `modUrl` | url | `getUrl()` — the mod's page on mods.vintagestory.at |
| `modSide` | string | the online `"type"` field (client / server) |
| `modThumbnail` | string | image-provider URL, or **empty string until the online info lands**, since the logo URL is only known then |
| `isFavoriteMod` | bool | `isFavorite()` |

Names come from `ModListModel::roleNames()` (`ModListModel.cpp:82`); types from
`ModListModel::data()` just above it.

Inside a delegate, reference them unqualified:

```qml
Label { text: modName }
Label { text: qsTr("v%1").arg(modVersion) }
Rectangle { visible: modHasUpdate }
```

> **Do not convert these to `required property` declarations in `ModEntry.qml`.** Doing so
> switches the view from context-property injection to object properties, and the nested
> components `ModEntry_Icon.qml` and `ModEntry_ButtonsSection.qml` — which read the roles out
> of the delegate's context — break at runtime with `ReferenceError`. This was tried and
> reverted on 2026-08-09.

---

## GameMngr

`src/gamemngr/GameMngr.hpp:34-77` · implements `IGameMngr` (`src/interfaces/IGameMngr.hpp:31-53`)

> As with `Config`, the property and its notify signal are declared on the abstract base, so
> grepping `GameMngr.hpp` for `gameVersion` finds nothing. QML sees it on the `GameMngr`
> singleton through the metaobject; `IGameMngr` is not a QML type.

| Property | Type | Notify | Meaning |
|---|---|---|---|
| `gameVersion` | `QString` (read-only) | `gameVersionChanged` | installed game version, **empty string when unknown** |

| Method | Effect |
|---|---|
| `launchGame()` | starts the Vintage Story executable from `Config.paths.gameExe` |

```qml
Button {
    text: qsTr("Launch Game")
    onClicked: GameMngr.launchGame()
}

Label {
    text: GameMngr.gameVersion ? qsTr("Vintage Story %1").arg(GameMngr.gameVersion)
                               : qsTr("Game version unknown")
}
```

The version is read by running the executable in a subprocess, so it is **not available at
startup** — bind to the property rather than reading it once. It is re-read whenever
`Config.paths.gameExe` changes, and reverts to an empty string if the read fails or times
out. Update detection is disabled while it is empty (`App.cpp:94` logs a critical).

`getModsDirs()` is C++-only.

---

## ModLoader

`src/modloader/ModLoader.hpp:32-82` · owns every route a zip takes into the store

No properties. One invokable, and it is the **only** way QML installs a mod:

| Method | Signature | Effect |
|---|---|---|
| `load(filePath)` | `url` | read the zip, then add it to the store as a manual install |

```qml
FileDialog {
    onAccepted: ModLoader.load(selectedFile)
}
```

`AddMod.qml` is the working reference. The loader extracts `modinfo.json` on a worker thread
and calls `ModStore::add` with a `GUI` load type, which installs the zip into the mods folder
and repoints the entry at that copy. If the mod id is already installed, the same call is
treated as an update instead, so installing a newer zip by hand replaces the old one.

Nothing is reported back to QML: a file that is not a readable zip, or a version not newer
than the installed one, is only logged. Everything else the loader does — the initial scan and
update downloads — is driven by signals from `ModStore`.

---

## Image provider

Registered as `modicon` in `App.cpp:64`, before `loadFromModule`, because a row can ask for an
icon as soon as QML loads. URLs are built by the model, not by you:

```
image://modicon/<modId>?url=<percent-encoded logofile>
```

The query carries the mod's logo URL from the online info, so a mod whose logo changes is
automatically a different QML source and Qt's image cache cannot serve the old one. There is no
cache-busting counter to maintain.

**Bind to the `modThumbnail` role rather than composing the URL yourself** — the model does the
percent-encoding (an unencoded nested URL loses everything after its first `&`), and returns an
empty string when no icon is available:

```qml
Image {
    source: modThumbnail
    visible: modThumbnail !== ""
}
```

`ModEntry_Icon.qml` shows the full pattern, including the fallback icon.

---

## Context properties

Set on the root context in `App.cpp:67-79`:

| Name | Type | True when |
|---|---|---|
| `IS_LINUX` | bool | built for Linux |
| `IS_WINDOWS` | bool | built for Windows |
| `IS_MACOS` | bool | built for macOS |

These are compile-time constants, not runtime detection.

Two caveats: context properties are **global and untyped**, and **invisible to `qmlls`**, so
using one produces an "unqualified access" warning and gets no completion. If platform
branching grows beyond a couple of sites, a typed singleton would serve better.

---

## Known rough edges

**Config settings are no longer untyped maps.** The `QVariantHash` sections that used to make
`qmllint` report `Member "gameConfig" not found on type "QVariantHash"` are now `Q_GADGET`
types with real properties, so member access resolves. What remains is the `theme` enum: it is
reachable only as a number until `vsmm::appearance::Theme` is registered as a QML type.

**No error channel.** Nothing surfaces failures to QML — a failed download, an unreadable
archive or a bad game path is only logged. There is no property or signal to bind an error
banner to. The sharpest case today is a manual install through `ModLoader.load()`: picking a
zip older than the installed version, or one whose copy into the mods folder fails, logs a
line and changes nothing, while `AddMod.qml` closes as if it had worked.

**Selection state is write-only from QML's side.** `markForUpdate()` sets it and
`modsSelected` reports whether *any* mod is selected, but there is no way to read back which
mods are selected, or to clear the selection in one call.
