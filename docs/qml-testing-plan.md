# QML testing for VSMM: harness, injection seam, and component tests

## Context

VSMM has solid C++ test coverage (`src/config/tests/ConfigTest.cpp`,
`src/modentry/tests/ModEntryTest.cpp` — QTest, `QSignalSpy`, data-driven cases,
`QStandardPaths::setTestModeEnabled`) but **zero coverage of the QML layer**, which is where
most of the recent work has gone and where real logic now lives: each settings tab's derived
`dirty` property, `Settings.qml`'s save/reset round-trip, and the combined update button's
state and `enabled` conditions.

### How QML testing is actually split

QML has no bespoke taxonomy — the usual pyramid applies, but the tiers are defined by which
*tool* runs them:

| Tier | Tool | What it covers | Status here |
|---|---|---|---|
| Unit (backend) | **QTest** (C++) | models, config, business logic | already done |
| Unit/component (QML) | **Qt Quick Test** — `TestCase` in QML | bindings, signals, states, exposed functions | **missing — this plan** |
| Integration | QTest driving `QQmlApplicationEngine`, or QML tests against real singletons | the C++↔QML contract | partially covered by the above |
| Functional / E2E | Squish (commercial); open-source options are thin | full user journeys | out of scope |

Qt Quick Test is the QML "unit" tier, though note it instantiates a real scene graph, so it
is closer to component-integration than a pure unit test. Its toolkit: `TestCase`,
`SignalSpy`, `compare`/`verify`, `tryCompare`/`tryVerify` (for bindings that settle
asynchronously), `createTemporaryObject()`, and `when: windowShown` for input-driven tests.

### Why an injection refactor comes first

`qmltestrunner` alone cannot test these components: they reach directly for the C++
singletons (`ModStore.` ×18, `Config.` ×14), which are registered via `QML_SINGLETON` in
their own modules. Tests would get real, engine-created instances — `Config` would read the
developer's actual config file, and `ModStore` would be empty and unsteerable.

The fix is a seam: leaf components receive their data through properties instead of reaching
for globals. Singleton knowledge collapses into the two top-level windows, everything below
becomes testable with plain QML fakes, and the components get better separation as a
by-product.

## Design: the injection seam

**Rule:** a component may reference a C++ singleton *only if* it is a top-level window
(`Main.qml`, `Settings.qml`). Everything else declares an injected provider:

```qml
// leaf component
property var config: null          // supplied by the parent
```

```qml
// Settings.qml — the only place that knows about the real singleton
SettingsTab_General { id: generalTab; config: Config; ... }
```

Default to `null`, not to the singleton itself: a default of `Config` keeps a hard type
dependency and drags the whole module graph into every test. With `null`, the tab has no
dependency at all and instantiates standalone.

Use `property var` rather than a typed property. A typed `property Config config` would keep
better editor completion, but a fake could then never be substituted — untyped is the price
of mockability. Note this trade-off in a comment so it does not look accidental.

## Implementation

### 1. Injection refactor (production code)

| File | Change |
|---|---|
| `src/qml/content_components/settings/SettingsTab_General.qml` | add `property var config: null`; route the three `Config.general.*` reads through it |
| `src/qml/content_components/settings/SettingsTab_Configuration.qml` | add `property var config: null`; route the `Config.paths.*` reads (incl. the two `TextEdit` "Current Path" bindings) through it |
| `src/qml/content_components/MainButtonsSection.qml` | add `property var modStore: null`; route `modsSelected` / `workPending` / `updatesCount` / `reload()` / `updateSelected()` / `updateAll()` through it |
| `src/qml/content_components/StatCards.qml` | add `property var modStore: null`; route `installedModsCount` / `updatesCount` through it |
| `src/qml/Settings.qml` | pass `config: Config` to all three tabs |
| `src/qml/Main.qml` | pass `modStore: ModStore` to `MainButtonsSection` and `StatCards` |

Guard bindings against the null default (`root.config ? root.config.general.x : false`) so
components still load standalone in the designer and in tests that do not supply a fake.

**Do not introduce any `id` beginning with `_`** — see the underscore/qmlls note below.

### 2. Test harness

New `src/qml/tests/` following the existing `src/*/tests/` convention:

- **`CMakeLists.txt`** — mirror `src/config/tests/CMakeLists.txt`: `project(QmlTests)`,
  `find_package(Qt6 REQUIRED COMPONENTS QuickTest Qml Quick)`, `qt_add_executable`,
  `add_test(NAME QmlTests COMMAND QmlTests -input ${CMAKE_CURRENT_SOURCE_DIR})`, and link the
  singleton libraries (`Config`, `ModStore`, `ModSortFilterModel`, `GameMngr`, `ModLoader`,
  `ModListModel`) so `import vsmm` resolves — the generated `vsmm` qmldir declares them as
  dependencies.
- **`main.cpp`** — `QUICK_TEST_MAIN_WITH_SETUP(QmlTests, Setup)` from `<QtQuickTest>`, with a
  `Setup` QObject providing:
  - `applicationAvailable()` → `QStandardPaths::setTestModeEnabled(true)` (same guard as
    `ConfigTest.cpp:56`, so no test can touch the real config) and a quiet message handler,
    mirroring `ConfigTest::quietHandler`.
  - `qmlEngineAvailable(QQmlEngine*)` → `addImportPath()` for `${CMAKE_BINARY_DIR}/qml_imports`
    and `src/qml/styles`, matching the paths already in `src/.qmlls.ini`.
- Register the subdirectory from `src/CMakeLists.txt` alongside the other test dirs.

### 3. Tests

Files are `tst_*.qml`; each defines fakes inline as plain `QtObject`s.

- **`tst_stringhelpers.qml`** — pure functions from `src/qml/js/StringHelpers.js`:
  `getCleanPath` (strips `file://`, handles the Windows `/C:` drive-letter case),
  `getFileName` (both separators), `isNullOrEmpty`, `isNullOrWhitespace`. Data-driven via
  `_data()` functions, matching the C++ tests' style.
- **`tst_settingstab_general.qml`** — inject a fake config; toggle each `Switch`; assert
  `dirty` becomes true when the value diverges from the saved one and false again when it
  matches, and that `revert()` restores both switches. `dirty` is a plain derived property,
  so `compare()` suffices — no `SignalSpy` needed.
- **`tst_settingstab_configuration.qml`** — same for `gameConfigDir` / `gameExePath`, including
  the cleared-path case (blank must be savable, so `dirty` must go true when a populated field
  is emptied).
- **`tst_mainbuttons.qml`** — the highest-value target, covering logic written this week:
  - `state === "hasSelection"` exactly when `modsSelected`
  - text / icon / tooltip / `defaultColor` all flip together with the state
  - `enabled` is false while `workPending`, false when nothing is selected and
    `updatesCount === 0`, true otherwise
- **`tst_settings_roundtrip.qml`** — inject a fake config into `Settings.qml`; assert
  `resetToCurrentConfig()` populates the tabs, `saveToConfig()` writes every field back
  verbatim (including cleared ones), and `settingsChanged` returns to false afterwards.
  Cover the cross-tab case too: dirtying one tab and reverting another must leave
  `settingsChanged` true.

## Verification

```sh
cmake -G Ninja -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
ctest --test-dir build-debug --output-on-failure      # runs C++ and QML tests together
ctest --test-dir build-debug -R QmlTests -V           # QML only, verbose
```

Then confirm nothing regressed at runtime — the injection refactor changes how every settings
tab and the main button row get their data:

```sh
./build-debug/bin/VSMM
```

Check: the mod list populates, stat cards show counts, the update button still flips between
"Update all" and "Update selected" and greys out while syncing, and Settings still loads and
saves paths.

Finally, re-check tooling health, since this adds new QML files:

```sh
qmllint -I build-debug/qml_imports -I src/qml/styles -I /usr/lib/qt6/qml src/qml/tests/*.qml
```

## Watch out for

- **Never write `id: _foo`.** A leading-underscore id makes qmlls return null semantic tokens
  for the entire file (Qt 6.11.1 bug); all 17 such ids were renamed on 2026-08-09. New test
  files must use plain names.
- **`ModEntry.qml` must not gain `required property` declarations for model roles** — that
  switches the view from context-property injection to object properties and breaks
  `ModEntry_Icon.qml` / `ModEntry_ButtonsSection.qml` at runtime. The injection rule in this
  plan applies to *singletons*, not model roles.
- Qt Quick Test needs a display; if CI is headless, run with `QT_QPA_PLATFORM=offscreen`.
