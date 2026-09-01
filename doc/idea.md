# Qanvas — Project Context

Runtime environment where an AI agent generates and runs QML apps backed by SQLite.

## What this is

Qanvas is a desktop runtime built on Qt/QML. The user describes an app in natural
language; an LLM agent writes a single `.qml` file; the runtime loads and runs it
immediately. No build step, no restart, no compilation.

The point is not "AI writes code for a developer." The point is a **system that
grows its own applications on demand** — small, personal tools that would never be
worth building by hand.

## Architecture

### Layers

```
┌─────────────────────────────────────┐
│  Generated app (.qml)               │  ← written by the agent, untrusted
├─────────────────────────────────────┤
│  Bridge (C++, QObject)              │  ← the only way out of the app
├─────────────────────────────────────┤
│  system.db        apps/<id>.db      │  ← two separate data layers
└─────────────────────────────────────┘
```

### Data model

Two layers, deliberately separated:

**`system.db`** — hand-written, stable schema. Shared domain entities (notes,
tasks, contacts, events) plus the app registry (`apps`, `app_permissions`).
The agent **never** issues SQL against this database. Access goes through
`Q_INVOKABLE` methods on the bridge: `Notes.list()`, `Notes.add(text)`, etc.
This is what makes generated apps interoperable rather than isolated toys.

**`apps/<app_id>.db`** — one SQLite file per generated app. The agent has full
freedom here, including DDL and dropping tables. Blast radius is exactly one app;
uninstalling an app means deleting one file.

### The bridge

Generated QML must not execute arbitrary SQL strings. The C++ bridge:

- accepts **parameterized queries only** — never concatenated strings
- rejects `ATTACH`, `DETACH`, and `PRAGMA` (an `ATTACH` on `system.db` defeats
  the entire separation)
- disables SQLite extension loading (`enable_load_extension(false)`)
- allows DDL only inside the app's own private database
- checks `app_permissions` before touching anything shared

`QtQuick.LocalStorage` gives a separate DB file per app name, which fits the
private layer — but it has no access control, so the shared layer needs the
custom bridge regardless.

## Generation pipeline

1. User request → agent → single `.qml` file
2. Run `qmllint` on the output
3. On failure, feed the error back to the model and retry (bounded retries)
4. On success, register in `apps`, create `apps/<id>.db`, load and run

Keep the model call behind a single interface so swapping models is a one-line
change.

### Notes on model choice

QML is a niche language with thin training-data coverage; small models struggle
with it more than with mainstream languages. A small/cheap model is fine for
validating the pipeline end to end (does the file load, does the sandbox hold),
but real apps need a stronger model.

Two things improve output quality more than upgrading the model:

- the `qmllint` retry loop above
- a handful of worked examples of the bridge API in the system prompt — the agent
  has never seen this API in training

## Design principles

- **The agent writes QML, nothing else.** No C++, no shell, no filesystem access.
- **Private data is disposable; shared data is protected.** Different rules per layer.
- **Start with one working app end to end,** not a general framework.
- **The bridge is the security boundary.** Everything hostile arrives as QML;
  assume the QML is wrong or malicious and make the bridge hold.

## Non-goals (for now)

- Multi-user / networked operation
- Sharing generated apps between installations
- Sandboxing beyond the SQL layer (no JS-level isolation yet)
- Anything requiring the agent to modify `system.db` schema