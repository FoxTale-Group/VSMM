# Backend API exposed to QML

Everything the C++ backend makes available to the QML layer: singletons, their properties,
invokable methods, model roles, the image provider, and context properties.

Compiled from the headers on 2026-08-10. Each entry cites its source file so it can be
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
(`src/App.cpp:74-90`).

### Overview

| Singleton | Module | Properties | Invokables | Use it for |
|---|---|---|---|---|
| `Config` | `Config` | 3 | — | reading / writing user settings |
| `ModStore` | `ModStore` | 4 | 8 | mod counts, selection state, all mutations |
| `ModSortFilterModel` | `ModSortFilterModel` | 1 | — | the model to feed a `ListView` |
| `ModListModel` | `ModListModel` | — | — | source model; defines the delegate roles |
| `GameMngr` | `GameMngr` | — | 1 | launching the game |
| `ModLoader` | `ModLoader` | — | — | nothing — see note below |

---

## Config

`src/config/Config.hpp:31-45` · persisted to `config.json` in `AppConfigLocation`

Three read/write properties, each a `QVariantHash` — a string→variant map, not a typed
object.

| Property | Type | Access | Notify |
|---|---|---|---|
| `general` | `QVariantHash` | read / write | `generalChanged` |
| `paths` | `QVariantHash` | read / write | `pathsChanged` |
| `appearance` | `QVariantHash` | read / write | `appearanceChanged` |

### Keys inside each hash

| Hash | Key | Type | Meaning |
|---|---|---|---|
| `general` | `deleteOldModVersion` | bool | remove the previous version when a newer one is added manually |
| `general` | `includeModPrerelease` | bool | consider pre-release versions when checking for updates |
| `paths` | `gameConfig` | string | the `VintagestoryData` folder |
| `paths` | `gameExe` | string | the Vintage Story executable |
| `appearance` | — | — | reserved; no keys defined yet (Settings → Appearance is still a stub) |

Key names come from `Config.hpp:44-45` and `GameMngr.hpp:34-35`.

### Reading

```qml
Label { text: Config.paths.gameConfig ?? "" }
```

Always guard with `??` — a key that has never been written is `undefined`.

### Writing — read, mutate a copy, assign back

`QVariantHash` is a **value type**. Mutating it in place does nothing; you must assign the
whole hash back so the setter runs and the notify fires:

```qml
let pathsCfg = Config.paths          // copy
pathsCfg.gameConfig = "/new/path"    // mutate the copy
Config.paths = pathsCfg              // assign back — this persists
```

`Settings.qml:117-135` is the working reference for this pattern.

### Not exposed to QML

`getFavorites()`, `setFavorites()`, `getPath()`, `validate()` and the templated
`getGeneral<T>()` / `getAppearance<T>()` are plain C++ members with no `Q_INVOKABLE` —
they exist for the backend only.

---

## ModStore

`src/modstore/ModStore.hpp:33-55` · the single entry point for anything that changes mods

### Properties — all read-only

| Property | Type | Notify | Meaning |
|---|---|---|---|
| `installedModsCount` | int | `modsChanged` | number of installed mods |
| `updatesCount` | int | `modsChanged` | how many have an update available |
| `workPending` | bool | `workChanged` | an update or a reload is in progress |
| `modsSelected` | bool | `modSelected` | at least one mod is checked for update |

`installedModsCount` and `updatesCount` **share one notify signal**, so both re-evaluate
whenever either changes. Harmless, but worth knowing if you ever profile bindings.

`workPending` starts as `true` (`ModStore.hpp:69`) — the store considers itself busy until
the first reload finishes, so anything gated on it is disabled at startup by design.

### Invokable methods

| Method | Signature | Effect |
|---|---|---|
| `reload()` | — | re-scan the mods directories |
| `load(filePath)` | `url` | install a mod from a local archive |
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
`modUpdated`, `modsReloading`, `modAddedFromGUI`, `modUpdateRequested`, `modRemoved` — are
internal wiring between the backend libraries (each is commented as such in the header).
They are technically reachable from QML, but treat them as private.

---

## ModSortFilterModel

`src/modsortfiltermodel/ModSortFilterModel.hpp:28-30` · a `QSortFilterProxyModel` over `ModListModel`

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

`src/modlistmodel/ModListModel.hpp:33-50` · the source model — bind views to
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
| `modThumbnail` | string | image-provider URL, or **empty string when no icon is cached** |
| `isFavoriteMod` | bool | `isFavorite()` |

Names come from `ModListModel::roleNames()` (`ModListModel.cpp:77-84`); types from
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

`src/gamemngr/GameMngr.hpp:31-44`

No properties. One invokable:

| Method | Effect |
|---|---|
| `launchGame()` | starts the Vintage Story executable from `Config.paths.gameExe` |

```qml
Button {
    text: qsTr("Launch Game")
    onClicked: GameMngr.launchGame()
}
```

`getModsDirs()` and `getGameVersion()` are C++-only — the game version is **not** currently
available to QML.

---

## ModLoader

`src/modloader/ModLoader.hpp:36-37`

Registered as a singleton but exposes **no properties and no invokable methods**, so there is
nothing to call from QML. It is driven entirely by signals from `ModStore`. Listed here only
so its presence in the import list isn't mistaken for an available API.

---

## Image provider

Registered as `modicon` in `App.cpp:57`. URLs are built by the model, not by you:

```
image://modicon/<modId>?diff=<cacheKey>
```

The `?diff=` parameter changes when a mod's icon is re-downloaded, which busts Qt's internal
image cache so the new icon actually appears.

**Bind to the `modThumbnail` role rather than composing the URL yourself** — the model already
handles the cache key, and returns an empty string when no icon is available:

```qml
Image {
    source: modThumbnail
    visible: modThumbnail !== ""
}
```

`ModEntry_Icon.qml` shows the full pattern, including the fallback icon.

---

## Context properties

Set on the root context in `App.cpp:60-72`:

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

**`QVariantHash` defeats tooling.** `Config.general` and `Config.paths` are untyped maps, so
`qmllint` reports `Member "gameConfig" not found on type "QVariantHash"` and you get no
completion on config keys. The warnings are expected and cannot be fixed from QML — it needs
the C++ side to expose a `Q_GADGET` with real properties, or individual `Q_PROPERTY`s.

**The game version isn't exposed.** `GameMngr::getGameVersion()` exists but has no
`Q_PROPERTY`, so the UI cannot show which game version is installed or warn about mod
compatibility.

**No error channel.** Nothing surfaces failures to QML — a failed download, an unreadable
archive or a bad game path is only logged. There is no property or signal to bind an error
banner to.

**Selection state is write-only from QML's side.** `markForUpdate()` sets it and
`modsSelected` reports whether *any* mod is selected, but there is no way to read back which
mods are selected, or to clear the selection in one call.
