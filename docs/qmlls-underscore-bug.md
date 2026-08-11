# qmlls: a leading underscore in a QML `id` kills tooling for the whole file

**Status:** reported upstream to Qt (2026-08-11). Issue link: [QTBUG-149089](https://qt-project.atlassian.net/browse/QTBUG-149089)

**Affects:** Qt 6.11.1 (qt6-declarative 6.11.1-3). Unfixed at time of writing.

## The rule for this codebase

> **Never write `id: _foo`.** Use a plain name — `id: foo`.

That is the whole practical takeaway. Everything below is why, and how to re-check it once
Qt ships a fix.

## What happens

If **any** object in a QML document declares an `id` starting with `_`, `qmlls` answers
`textDocument/semanticTokens/full` with `null` for that document. The effect is total for
the file — no semantic highlighting, no completion, for every symbol in it, not just the id.

The id does not have to be referenced anywhere. Declaring it is enough.

Three things make this nasty to diagnose:

- **It is legal QML.** The docs state an id "must begin with a lower-case letter or an
  underscore".
- **The runtime is unaffected.** The app builds and runs correctly; only the language
  server breaks. So nothing in the build output hints at it.
- **It is per-file and silent.** It presents as "some files have completion, some don't",
  which looks like a broken editor setup rather than a code problem.

Underscore-prefixed **property** names are fine — `Theme.qml` still has `_iconsPath` inside
`IconSet` and analyses normally. Only `id` triggers it.

`qmllint` is unaffected (exits 0 either way), so it cannot be used to detect this.

## What it cost us

Found 2026-08-11 after roughly two days of misdiagnosis. Before isolating the underscore we
wrongly blamed — and "fixed" — import paths, the flatpak sandbox, `states`/`PropertyChanges`,
`required property` declarations, and id-qualified `modelData` access. All were innocent.

The project had 17 underscore-prefixed ids across 12 files. Correlation with broken files was
exact: all 10 source files containing one returned `null`; every other file returned tokens.

| File | with `_id` | after rename |
|---|---|---|
| `Theme.qml` (`_ThemeRoot` → `themeRoot`) | `null` | 737 tokens |
| `ModEntry.qml` (`_modEntry` → `modEntryRoot`) | `null` | 297 tokens |

Two renames needed more than dropping the underscore:

- `_modEntry` → **`modEntryRoot`**, not `modEntry`: `VSMMStyle/Button` has a `modEntry`
  property, and `ModEntry_ButtonsSection.qml` reads ids out of the delegate's context, so
  the shorter name risked a collision.
- `_SettingsTab_*` → `settingsTabGeneral` etc.: stripping the underscore alone would leave a
  capital first letter, which QML forbids for ids.

## Minimal reproducer

No project, build directory, or import paths needed. Two files differing by one character.

`tst_underscore.qml`
```qml
import QtQuick

Item {
    Item { id: _probe }
}
```

`tst_plain.qml`
```qml
import QtQuick

Item {
    Item { id: probe }
}
```

Result — deterministic, 3/3 runs each way:

| File | `semanticTokens/full` |
|---|---|
| `tst_underscore.qml` | **`null`** |
| `tst_plain.qml` | 6 tokens |

## Re-checking after a Qt update

Save the two files above plus this script, then run
`python3 probe.py tst_underscore.qml` and `python3 probe.py tst_plain.qml`.
The bug is fixed when both report tokens.

```python
# probe.py -- minimal stdio LSP client: opens one QML file and asks qmlls for
# semantic tokens. Prints the token count, or NULL if the server returns nothing.
import json, subprocess, sys, time, os
f = os.path.abspath(sys.argv[1])
root = os.path.dirname(f)
p = subprocess.Popen(["/usr/lib/qt6/bin/qmlls"], stdin=subprocess.PIPE,
                     stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
def send(m):
    b = json.dumps(m).encode()
    try:
        p.stdin.write(b"Content-Length: %d\r\n\r\n" % len(b) + b); p.stdin.flush()
    except BrokenPipeError:
        print(f"  {os.path.basename(f)}: SERVER CRASHED"); raise SystemExit
def read():
    h = b""
    while b"\r\n\r\n" not in h:
        c = p.stdout.read(1)
        if not c: return None
        h += c
    n = int([l for l in h.decode().split("\r\n") if l.lower().startswith("content-length")][0].split(":")[1])
    return json.loads(p.stdout.read(n))
send({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"processId":None,
  "rootUri":"file://"+root,"capabilities":{"textDocument":{"semanticTokens":{
   "requests":{"full":True},"tokenTypes":[],"tokenModifiers":[],"formats":["relative"]}}}}})
read(); send({"jsonrpc":"2.0","method":"initialized","params":{}})
send({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{
    "uri":"file://"+f,"languageId":"qml","version":1,"text":open(f).read()}}})
time.sleep(5)
send({"jsonrpc":"2.0","id":100,"method":"textDocument/semanticTokens/full",
      "params":{"textDocument":{"uri":"file://"+f}}})
end = time.time()+20
while time.time() < end:
    m = read()
    if m is None:
        print(f"  {os.path.basename(f)}: SERVER DIED"); break
    if m.get("id") == 100:
        r = m.get("result")
        print(f"  {os.path.basename(f)}: {'NULL (no tokens)' if not r or not r.get('data') else str(len(r['data'])//5)+' tokens'}")
        break
p.kill()
```

## Related crashes

In the full project — but **not** in the minimal reproducer, which returns `null` without
dying — `qmlls` also segfaulted repeatedly while editing affected files: 8+ coredumps over
two weeks. Possibly the same root cause, possibly separate; it could not be reduced to a
minimal case. Two distinct top frames were seen, both inside the import path:
`QQmlJSUtils::fileSelectorFor`, and unbounded mutual recursion between
`QQmlJSImporter::importHelper` and `importDependencies`.

If qmlls dies repeatedly, editors stop restarting it after 5 crashes in 3 minutes and need a
manual "Restart QML Language Server".
