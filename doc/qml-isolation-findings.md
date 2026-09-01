# QML Isolation — Measured Findings

Notes on running agent-generated QML inside the Qanvas runtime. This document
records **what each mechanism actually does**, not which one the project picks.
Everything below marked *measured* was observed on a real run; everything marked
*not evaluated* was not.

## How this was measured

Two throwaway probe applications, neither kept in this repository:

- **Android** — a self-contained Qt6 APK (Qt 6.10, arm64-v8a, real device). Two
  engines, an item created by engine B reparented into engine A's window,
  results written both to the screen and to a log file in the app's data dir.
  Worth remembering for the next probe: Qt messages land in logcat under tag
  `default` (`qInfo`) and `qml` (`console.log`) — *not* under the app name, so
  `adb logcat -s <appname>` shows nothing.
- **Desktop** — a host/guest application on Qt 6.4.2 / Wayland with a real scene
  graph (threaded, basic and default render loops; ASAN clean).

## 1. Two QQmlEngine instances in one process

*Measured, desktop and Android, identical results on both.*

- Two `QQmlEngine` instances coexist in one process with no warnings at all.
- An item created by engine B can be reparented into engine A's
  `QQuickWindow::contentItem()` via `setParentItem()`. It renders, animates and
  its bindings stay live. Zero Qt messages are emitted during the reparent.
- `QQmlEngine::contextForObject(item)->engine()` correctly reports engine B for
  the item and engine A for the window — the ownership split is real, not a
  side effect of the reparent.
- Signals cross the boundary in both directions through C++
  (`setProperty()` / `connect(..., SIGNAL/SLOT)`). There are no bindings between
  engines; the C++ hop is the only channel.

### Deleting an engine while its item is still in the scene

*Measured, desktop and Android.*

- The process does not crash. The item stays parented and visible, and any
  `QQuickRotationAnimation`-style animation keeps running (the animation lives on
  `QUnifiedTimer` in C++ and does not need the engine).
- The item is nevertheless dead: property writes are silently swallowed,
  `item->property("n")` reads back `0`, and `onNChanged` never fires again.
- Consequence: an engine and the items it created must be destroyed together.

## 2. What is scoped to the QQmlEngine

These knobs exist on `QQmlEngine` and cannot be varied per app inside one engine:

- `setNetworkAccessManagerFactory()` — one NAM per engine. QML's
  `XMLHttpRequest` goes through it, **including `file://` reads**, so per-engine
  NAM is what makes "network" expressible as a permission.
- `setImportPathList()` — determines which QML modules an app can `import`.
  One engine means every app sees the same set of modules.
- `addUrlInterceptor()`.
- `QJSEngine::setInterrupted()` — aborts JS execution on that engine only. In a
  single engine, the kill switch for a looping app also kills every other app and
  the runtime's own UI.
- Type registry and QML singletons. `qmlRegisterType` and `pragma Singleton` are
  engine-global; a host singleton with state is shared by every app in that
  engine.
- Component cache. `clearComponentCache()` is engine-wide, so reloading one
  app's file invalidates the cache of everything else in that engine.
- Deterministic teardown. `delete engine` frees the app's state at a known
  moment; inside a shared engine the state depends on GC and any forgotten
  reference keeps the app alive.

Startup cost measured on desktop: ~178 µs per app on a shared engine versus
~688 µs with an engine of its own. Memory, not time, is the real cost — each
engine carries its own V4 heap and its own copies of singletons.

### What a second engine does *not* buy

*Measured.*

- **Bridge identity per app does not need a second engine.** A bridge exposed on
  a child `QQmlContext` (not the root context) is already per-app in a single
  engine: app 1 sees app 1's data, app 2 sees app 2's.
- **No side channel between apps in one engine via the JS globals that were
  tried.** Writing to `Qt.__someProperty` from app 1 reads back `undefined` in
  app 2, and `globalThis` is not defined at all in QML
  (`ReferenceError: globalThis is not defined`). Note this was tested for ad-hoc
  globals only — QML singletons (above) *are* a shared channel and were not part
  of that test.

## 3. What is scoped to the window / item tree

*Measured, one shared window, guest running in its own engine.*

A guest item parented into the host's `contentItem` can walk `parent` up to the
root and enumerate `children` from there. In the measurement the guest:

- read a property off an unrelated host item (the text of a permission prompt),
- wrote to it (`"PODMIENIONE PRZEZ APKE"`),
- set `visible = false` on the runtime's own chrome.

`Window.window` likewise hands the guest the host window object.

This is plain `QQuickItem` traversal in C++, so a separate engine does not affect
it in any way. Engine and window are orthogonal axes: **the engine bounds the API
surface, the item tree bounds the scene.**

Giving each app its own top-level `QQuickWindow` severs the chain — the app's
tree root is its own window's `contentItem` and there is nothing above it to
reach.

## 4. Android

Android gives one usable top-level window (a single Activity/surface), so
"window per app" is not available there.

- Everything in section 1 and 2 behaves identically to desktop — verified on
  device. Per-engine NAM, import paths, `setInterrupted`, singleton instances and
  deterministic teardown are all intact on Android.
- Only the item-tree isolation from section 3 is lost. With one window, a
  generated app can repaint the screen and impersonate the runtime.
- Two consequences follow directly from the measurement in section 3:
  - a permission prompt rendered as QML in that window can be rewritten by the
    app it is asking about, so it cannot be trusted as a consent surface;
  - anything privileged that the runtime's own QML holds in a property is
    reachable by tree traversal, whereas a context property on the app's engine
    is not.
- Running one app at a time (destroying the previous subtree before creating the
  next) removes the "app A reads app B" case, since two apps are never in the
  tree together.

## 5. The host's QML module is on the app's import path

*Measured.* `qt_add_qml_module` registers the runtime's own types into the
`Qanvas` module. A loaded app is created on the host's engine, so that module is
on its import path too: `import Qanvas` from `apps/<id>/Main.qml` resolves, and a
host component (`Panel`) instantiates inside the app's subtree exactly as the
app's own sibling files do.

Read one way this is a feature — it is how the runtime would offer apps a shared
component library, without each generated app carrying a copy.

Read the other way it is a hole, because **every** type in the module is
reachable, `AppLoader` included:

```qml
import Qanvas
AppLoader { appId: "some-other-app" }   // status Ready, inside the app's subtree
```

*Measured:* the nested loader loads, which means an app can name another app and
get that app's subtree — and with it the `AppContext` bound to that app's
database. The bridge stops being "the only way out of the app".

Nothing about the engine closes this; the module has to be kept off the app's
import path, or the types in it have to refuse to be built from an app's context.
A cheap version of the second: `AppLoader` refuses to load when the context it
was created in is not the engine's root context.

## 6. What none of the above closes

- `Qt.exit()` / `Qt.quit()` from generated QML.
- `Qt.createComponent("file:///absolute/path.qml")` — import paths do not
  constrain absolute URLs.
- A binding loop or a busy loop on the GUI thread. `setInterrupted` covers JS
  execution only.
- Memory exhaustion.

All of these are process-scoped; only a process per app bounds them.

## 7. Mechanisms not evaluated

- **`QQuickRenderControl`** — the app gets its own `QQuickWindow` that is never
  shown, rendered to a texture that the host composites into the single real
  window. This restores disjoint item trees on Android. Cost: input events must
  be forwarded by hand (keyboard and IME on Android being the hard part) and GPU
  memory per app.
- **One OS process per app** — the only boundary that covers section 5. On
  desktop it turns the bridge into IPC; on Android it is impractical.

Both sit at the same seam in the design: they only stay cheap to adopt if the
bridge carries **serialisable values only** — `QVariant`, lists, maps — and never
hands a `QObject*` to the app.
