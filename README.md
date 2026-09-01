# qanvas

A Qt/QML runtime where an LLM writes small apps on demand — each sandboxed with its own SQLite database.

Right now this is the bare skeleton: a Qt Quick "Hello World" that builds for desktop and
packages for Android.

## Layout

| Path | What it is |
| --- | --- |
| `src/` | Application sources — `main.cpp` and the `Qanvas` QML module (`src/qml/`) |
| `android/` | Android package source dir: manifest, launcher icons, `strings.xml` |
| `libs/` | Shared component libraries, git submodule ([app-libs](https://github.com/dpietruchowski/app-libs)) — checked out, not built yet |
| `third_party/android_openssl/` | Prebuilt OpenSSL for Android, git submodule ([KDAB](https://github.com/KDAB/android_openssl)) — wired up but commented out |
| `Dockerfile.android` | Qt 6.10 + Android SDK 36 build image |

## Clone

```bash
git clone --recurse-submodules git@github.com:dpietruchowski/qanvas.git
# or, in an existing clone:
git submodule update --init --recursive
```

## Build — desktop

```bash
cmake -S . -B build-desktop -DCMAKE_BUILD_TYPE=Debug
cmake --build build-desktop
./build-desktop/src/appqanvas
```

## Build — Android

Everything runs inside the Docker image, no local Android SDK/NDK needed.

```bash
./build.sh                    # release, default target
./build.sh clear debug apk    # wipe build-android/, debug build, APK
./build.sh release aab        # release App Bundle
```

Artifacts land in `build-android/`.

## Signing

```bash
./generate_key.sh   # one-off: creates android/qanvas.keystore (gitignored)
./sign.sh           # signs build-android/.../android-build-release-unsigned.apk
```

## Formatting

```bash
cmake --build build-desktop --target format        # apply clang-format to src/
cmake --build build-desktop --target format_check  # verify only
```

## Version bump

`QT_ANDROID_VERSION_CODE` / `QT_ANDROID_VERSION_NAME` in the top-level `CMakeLists.txt` are the
single source of truth — the manifest placeholders and `sign.sh` both read from there.
