# qanvas

A Qt/QML runtime where an LLM writes small apps on demand. You describe what you need ("something to track which plants I watered"), an agent writes a `.qml` app, and the runtime loads it immediately. No build step, no restart. Each app runs in its own sandbox with its own SQLite database.

The idea isn't to help a developer write code faster. It's a system that grows its own small, personal tools, the kind that would never be worth building by hand.

Runs on desktop and Android. Work in progress.

## How it works

The agent only writes QML, nothing else: no C++, no shell, no file system access. Everything a generated app can reach goes through C++ bridges, and the bridges are the security boundary. The runtime assumes the QML may be wrong or even malicious.

Data lives in two separate layers:

- **Each app's own database** (`app.db`). The app has full control here, including creating and dropping tables. If something goes wrong, only that app is affected, and uninstalling it means deleting one file.
- **The shared `system.db`**, with notes, tasks and the app registry. Apps never send SQL to it. They call bridge methods such as `Notes.add(text)`, and only if they have permission. That's what lets generated apps share data instead of being isolated toys.

Apps are built from a library of around 40 ready-made components (`QanvasList`, `QanvasChart`, `QanvasDatePicker`, `QanvasStopwatch`, …) that all share one theme, so every generated app looks consistent without the agent having to style anything.

The agent's system prompt works as a state machine (route → design → build, inspect → modify, fix, debug) with worked examples. A test extracts every example from the prompt and checks that it really loads. After each change the runtime gives the agent feedback: QML warnings, unused files, failed writes, and elements that don't fit on the screen. If something is left unresolved at the end of a turn, the runtime asks the agent to fix it.

The agent can use OpenAI, OpenRouter or Claude. API keys are kept in the system's secure storage.

## Building

Qt 6, CMake and a C++20 compiler. Shared code comes from [app-libs](https://github.com/dpietruchowski/app-libs), included as a git submodule.

```bash
git clone --recurse-submodules git@github.com:dpietruchowski/qanvas.git
cd qanvas
cmake -B build-desktop -DCMAKE_BUILD_TYPE=Debug
cmake --build build-desktop -j$(nproc)
./build-desktop/src/appqanvas
```

Source and QML files are collected when CMake configures, so after adding or moving a file run `cmake -B build-desktop` again.

Android and AppImage builds use the scripts from app-libs, which read their settings from `app.env`. The Android build runs in Docker, so you don't need a local SDK or NDK.

```bash
./libs/scripts/build-android.sh debug apk
./libs/scripts/build-android.sh release aab
./libs/scripts/sign-apk.sh            # after running generate-keystore.sh once
./libs/scripts/build-appimage.sh
```

## Tests

```bash
ctest --test-dir build-desktop
python3 test/prompt_example.py   # runs the examples from the system prompt, needs the Debug build
```

## UI automation

Debug builds include a small TCP server that lets scripts drive the app by `objectName`:

```bash
python3 libs/tools/ui_session.py start --platform offscreen
python3 libs/tools/ui_driver.py click navApp
python3 libs/tools/ui_driver.py screenshot tmp/app.png
python3 libs/tools/ui_session.py stop
```

## Formatting

```bash
cmake --build build-desktop --target format         # apply clang-format to src/
cmake --build build-desktop --target format_check   # check only
```
