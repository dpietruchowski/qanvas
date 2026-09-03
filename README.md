# qanvas

A Qt/QML runtime where an LLM writes small apps on demand — each sandboxed with its own SQLite database.

## Layout

| Path | What it is |
| --- | --- |
| `app.env` | Every app parameter: name, targets, version, Android package, keystore, AppImage metadata. CMake reads it through `app_load_env()`, the scripts in `libs/scripts` source it |
| `src/main.cpp` | Entry point: engine, `QmlRegistrator`, the `Theme` singleton, the runtime types, the automation server |
| `src/runtime/` | The runtime library `qanvas_runtime`: apps on disk, `system.db` stores, host bridges, the chat agent and its tools |
| `src/qml/` | The host shell, QML module `Qanvas` |
| `src/components/` | `Theme` and the `Qanvas*` components agent-written apps are built from, QML module `Qanvas.Components` |
| `src/tests/` | GoogleTest suite, built with `BUILD_TESTING` (on by default in Debug) |
| `test/` | A hand-written test app and `prompt_example.py`, which runs the examples from the system prompt in the real runtime |
| `android/` | Android package source dir: manifest, launcher icons, `strings.xml` |
| `libs/` | Shared foundation, git submodule ([app-libs](https://github.com/dpietruchowski/app-libs)): C++ under `libs/cpp/`, QML under `libs/qml/`, CMake helpers, build scripts, UI automation tools, Claude skills |
| `third_party/android_openssl/` | Prebuilt OpenSSL for Android, git submodule ([KDAB](https://github.com/KDAB/android_openssl)) — not wired up yet |

## Clone

```bash
git clone --recurse-submodules git@github.com:dpietruchowski/qanvas.git
# or, in an existing clone:
git submodule update --init --recursive
```

## Build — desktop

```bash
cmake -B build-desktop -DCMAKE_BUILD_TYPE=Debug
cmake --build build-desktop -j$(nproc)
./build-desktop/src/appqanvas
```

Sources and QML files are globbed at configure time: after adding or moving a file, run `cmake -B build-desktop` again.

## Tests

```bash
ctest --test-dir build-desktop
python3 test/prompt_example.py   # needs the Debug build above
```

## Build — Android and AppImage

Both come from the shared scripts, which read `app.env`. The Android build runs inside the Docker image from `libs/docker`, no local SDK/NDK needed.

```bash
./libs/scripts/build-android.sh debug apk     # build-android/
./libs/scripts/build-android.sh release aab
./libs/scripts/sign-apk.sh                    # after generate-keystore.sh once
./libs/scripts/build-appimage.sh              # dist/
```

## UI automation

Debug builds link `app_automation`, a localhost TCP server that drives the app by `objectName`. `.ui_automation.json` names the binary, the port and the env var (`APP_AUTOMATION_PORT`).

```bash
python3 libs/tools/ui_session.py start --platform offscreen
python3 libs/tools/ui_driver.py find ""
python3 libs/tools/ui_driver.py click navApp
python3 libs/tools/ui_driver.py screenshot tmp/app.png
python3 libs/tools/ui_session.py stop
```

## Formatting

```bash
cmake --build build-desktop --target format        # apply clang-format to src/
cmake --build build-desktop --target format_check  # verify only
```

The style is `libs/.clang-format`; the root `.clang-format` is a symlink to it.

## Version bump

`VERSION_NAME` and `VERSION_CODE` live in `app.env`. `./libs/scripts/build-android.sh … official` bumps, commits and tags on its own.
