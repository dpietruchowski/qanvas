"""Runs the example app from the system prompt in the real runtime.

Extracts the create_app / write_file / edit_file steps of EXAMPLE 1 and 2 from
src/runtime/chat/systemprompt.txt, seeds an app with them, launches appqanvas
offscreen and reports whether it loads and what the database looks like after
each step. Needs a debug build in build-desktop (the UI automation server).

    python3 test/prompt_example.py
"""

import os
import re
import shutil
import sqlite3
import sys
import tempfile
import textwrap
import time

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, REPO + "/libs/automation")
from ui_driver import UiDriver

BINARY = REPO + "/build-desktop/src/appqanvas"
SCRATCH = REPO + "/tmp"
PROMPT = REPO + "/src/runtime/chat/systemprompt.txt"
APP = "shopping-list"

STEP = re.compile(r'^(B\d|M\d)\s+(create_app|write_file|edit_file)\b')
PATH = re.compile(r'path="([^"]+)"')


def strip_indent(lines):
    return "\n".join(line[4:] for line in lines)


def parse_steps(text):
    lines = text.split("\n")
    steps = []
    i = 0
    while i < len(lines):
        match = STEP.match(lines[i])
        if not match:
            i += 1
            continue
        tool = match.group(2)
        found = PATH.search(lines[i])
        path = found.group(1) if found else "Main.qml"
        i += 1
        if tool == "create_app":
            sections = {}
            current = None
            while not lines[i].startswith("    -> "):
                if lines[i].strip() in ("qml=", "data="):
                    current = "Main.qml" if lines[i].strip() == "qml=" else "Data.qml"
                    sections[current] = []
                else:
                    sections[current].append(lines[i])
                i += 1
            for name, body in sections.items():
                steps.append((tool, name, textwrap.dedent("\n".join(body)).strip("\n") + "\n"))
        elif tool == "write_file":
            body = []
            while not lines[i].startswith("    -> "):
                body.append(lines[i])
                i += 1
            steps.append((tool, path, textwrap.dedent("\n".join(body)).strip("\n") + "\n"))
        else:
            assert lines[i].strip() == "before=", lines[i]
            i += 1
            before = []
            while lines[i].strip() != "after=":
                before.append(lines[i])
                i += 1
            i += 1
            after = []
            while not lines[i].startswith("    -> "):
                after.append(lines[i])
                i += 1
            steps.append((tool, path, (strip_indent(before), strip_indent(after))))
    return steps


def apply(root, steps):
    app = root + "/" + APP
    os.makedirs(app, exist_ok=True)
    for tool, path, payload in steps:
        target = app + "/" + path
        if tool == "edit_file":
            before, after = payload
            source = open(target).read()
            assert source.count(before) == 1, (path, before, source.count(before))
            payload = source.replace(before, after)
        with open(target, "w") as f:
            f.write(payload)


def run(root, config, label):
    driver = UiDriver.from_config(repo_root=REPO)
    driver.launch(BINARY, env={"QANVAS_APPS_ROOT": root, "QT_QPA_PLATFORM": "offscreen", "XDG_CONFIG_HOME": config})
    time.sleep(1.0)
    try:
        driver.click("navApp")
        time.sleep(1.5)
        driver.set("appsPage", "openedId", APP)
        time.sleep(1.5)
        status = driver.get("appLoader", "status")
        error = driver.get("appLoader", "errorString")
    finally:
        driver.terminate()
    db = sqlite3.connect(root + "/" + APP + "/app.db")
    schema = db.execute("SELECT sql FROM sqlite_master WHERE name = 'items'").fetchall()
    db.close()
    print(label, "loader:", status, repr(error))
    print(label, "schema:", schema)
    return status == "Ready"


def seed(root):
    db = sqlite3.connect(root + "/system.db")
    db.execute("CREATE TABLE apps (id TEXT PRIMARY KEY, title TEXT NOT NULL, created_at TEXT NOT NULL, updated_at TEXT NOT NULL)")
    db.execute("INSERT INTO apps VALUES (?, 'Lista zakupow', '2026-09-01T00:00:00Z', '2026-09-01T00:00:00Z')", (APP,))
    db.commit()
    db.close()


def main():
    steps = parse_steps(open(PROMPT).read())
    first_edit = next(i for i, step in enumerate(steps) if step[0] == "edit_file")
    os.makedirs(SCRATCH, exist_ok=True)
    root = tempfile.mkdtemp(prefix="qanvas-apps-", dir=SCRATCH)
    config = tempfile.mkdtemp(prefix="qanvas-config-", dir=SCRATCH)
    ok = True
    try:
        seed(root)
        apply(root, steps[:first_edit])
        ok &= run(root, config, "example1")
        db = sqlite3.connect(root + "/" + APP + "/app.db")
        db.execute("INSERT INTO items (text) VALUES ('mleko')")
        db.commit()
        db.close()
        apply(root, steps[first_edit:])
        ok &= run(root, config, "example2")
        ok &= run(root, config, "example2-again")
        db = sqlite3.connect(root + "/" + APP + "/app.db")
        print("rows:", db.execute("SELECT id, text, done, qty FROM items").fetchall())
        db.close()
    finally:
        shutil.rmtree(root, ignore_errors=True)
        shutil.rmtree(config, ignore_errors=True)
    print("OK" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
