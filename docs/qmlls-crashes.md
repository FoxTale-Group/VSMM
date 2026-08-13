# qmlls SIGSEGVs on 6.11.1: two distinct crashes

**Status:** open. Reported under
[QTBUG-149089](https://qt-project.atlassian.net/browse/QTBUG-149089) alongside the
underscore-id bug, but **unrelated to it**. The underscore bug is fixed for 6.11.2; these are
not. A Qt maintainer asked for full stack traces on 2026-08-13; both are attached to the issue.

**Environment:** Arch Linux, `qt6-declarative 6.11.1-3`, `qt6-base 6.11.1-1`. Client is CLion,
which launches `qmlls` with only `-b <build dir>`. Crashes happened while editing QML in a
project that was concurrently being reconfigured/rebuilt.

18 core dumps were retained from 2026-08-09 and are still readable via `coredumpctl`.

## Crash A: corrupt Qt container read inside `importHelper`

Seen in 3 cores. A worker thread faults reading a Qt container while populating builtins:

```
#0  QArrayDataPointer<QString>::constEnd        qarraydatapointer.h:125
#2  QList<QString>::append                      qlist.h:535
#5  operator()                                  qqmljsimporter.cpp:847  (lambda from :840)
#7  QQmlJSImporter::importHelper                qqmljsimporter.cpp:946
#9  QQmlJSImporter::builtinInternalNames        qqmljsimporter.cpp:824
#10 QQmlJSImportVisitor::QQmlJSImportVisitor    qqmljsimportvisitor.cpp:169
#13 DomEnvironment::populateFromQmlFile         qqmldomtop.cpp:2297
#19 QDeferredFactory<QQmlJSScope>::populate     qqmljsscope.cpp:1234
#22 QmlFile::ensurePopulated                    qqmldomexternalitems_p.h:475
#33 QQmlCodeModel::newDocForOpenFile(…)::lambda qqmlcodemodel.cpp:592
```

The other two cores fault in the same region on different containers:
`QArrayDataPointer<char16_t>` (a `QString` copy) and
`QHashPrivate::iterator<Node<QString, QQmlJS::ContextualType>>::isUnused`.

In one core the **main thread was concurrently rebuilding the resource mapper**:
`onBuildFinished` → `QQmlCodeModel::setResourceFiles` → `DomEnvironment::setResourceFiles`
(`qqmldomtop.cpp:2221`) → a fresh `QQmlJSResourceFileMapper`.

**Do not overstate this.** The two threads hold *different* `DomEnvironment` instances
(`0x7fd29c034570` worker vs `0x55ecd3471990` main), so it is not a demonstrated same-object
race. What is suggestive is that three cores fault on corrupt container *contents* rather than
on bad input, coinciding with a rebuild.

## Crash B: garbage `QQmlJSScope*` in the `QQmlJSTypeResolver` constructor

Seen in 2 cores with byte-identical stacks, so it is deterministic.

```
#0  Qt::totally_ordered_wrapper<QQmlJSScope*>::get   qcomparehelpers.h:1101   this=0x248
#2  QDeferredSharedPointer<QQmlJSScope>::operator QDeferredSharedPointer<const QQmlJSScope>
#3  QQmlJSScope::listType
#4  QQmlJSTypeResolver::QQmlJSTypeResolver
#11 std::make_shared<QQmlJSTypeResolver, QQmlJSImporter*>
#12 DomEnvironment::populateFromQmlFile
```

`this=0x248` reads like a member offset from a null base, i.e. `listType()` going through a
null/uninitialised deferred pointer while the type resolver is being constructed.

## What both share

Both are reached by a **worker thread** entering `DomEnvironment::populateFromQmlFile` via
`QDeferredFactory<QQmlJSScope>::populate` / `QmlFile::ensurePopulated`, i.e. lazy population
triggered from `newDocForOpenFile`.

## Reproducing a backtrace from a core

Arch's debuginfod supplies Qt debug symbols, so no rebuild is needed:

```sh
coredumpctl list | grep qmlls                    # pick a PID
coredumpctl dump <pid> --output=/tmp/core.<pid>
gdb -q /usr/lib/qt6/bin/qmlls /tmp/core.<pid> \
    -iex "set debuginfod enabled on" -iex "set pagination off" \
    -ex "thread apply all bt" -ex "thread 1" -ex "bt full" -batch
```

Expect `build-id does not match` warnings for glibc/libstdc++ if those were updated after the
dump. Those are harmless, the Qt frames still resolve. Prefer the largest cores; the 2.2 MB
ones tend to have shallower stacks than the 11-34 MB ones.

Generated traces and the drafted issue comment are in `~/qtbug-149089/` (outside the repo,
since they are large and machine-specific).
