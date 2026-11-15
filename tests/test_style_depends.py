#!/usr/bin/env python3
"""A package whose QML imports org.raye.ui depends on raye-style, which
ships that module: without it the window never loads.

    python3 tests/test_style_depends.py
"""
import pathlib
import re
import sys
import unittest

REPO = pathlib.Path(__file__).resolve().parent.parent
APP = re.compile(r"^raye_add_app\((\w+)\s+PACKAGE\s+([\w.-]+)(?![^)]*NOT_WRITTEN_YET)", re.M)
EXTRA = {"portal": "xdg-desktop-portal-raye"}


def depends(package):
    control = (REPO / "debian" / "control").read_text()
    stanza = re.search(rf"^Package: {re.escape(package)}\n(.*?)(?:\n\n|\Z)", control, re.M | re.S)
    if not stanza:
        return ""
    m = re.search(r"^Depends:(.*?)(?=^\S)", stanza.group(1) + "\nX", re.M | re.S)
    return m.group(1) if m else ""


class StyleDepends(unittest.TestCase):
    def test_apps_using_the_style_depend_on_it(self):
        folders = dict(APP.findall((REPO / "CMakeLists.txt").read_text()))
        folders.update(EXTRA)
        self.assertIn("taskmanager", folders)
        missing = []
        for folder, package in sorted(folders.items()):
            uses = any("import org.raye.ui" in q.read_text() for q in (REPO / folder).rglob("*.qml")
                       if "tests" not in q.relative_to(REPO).parts)
            if uses and not re.search(r"\braye-style\b", depends(package)):
                missing.append(package)
        self.assertEqual(missing, [])


if __name__ == "__main__":
    sys.exit(unittest.main())
