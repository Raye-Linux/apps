#!/usr/bin/env python3
"""What the apps show speaks Linux: root access, sudo user, never
administrator. Comments and identifiers keep the old word; the search
keywords of a .desktop file keep it too, since people coming from Windows
type it.

    python3 tests/test_root_terms.py
"""
import pathlib
import re
import sys
import unittest

REPO = pathlib.Path(__file__).resolve().parent.parent
OLD = re.compile(r"administrat|\badmin\b", re.I)
CALL = re.compile(r'\bi18n[a-z]*\((?:"[^"]*",\s*)?"([^"]*)"')


def sources(*patterns):
    for pattern in patterns:
        for path in sorted(REPO.rglob(pattern)):
            parts = path.relative_to(REPO).parts
            if path.is_file() and not {".git", "debian", "obj-x86_64-linux-gnu", "build"} & set(parts) \
                    and "tests" not in parts:
                yield path


class RootTerms(unittest.TestCase):
    def test_i18n_strings(self):
        bad = [f"{p.relative_to(REPO)}: {s}" for p in sources("*.qml", "*.js", "*.cpp")
               for s in CALL.findall(p.read_text(encoding="utf-8", errors="replace")) if OLD.search(s)]
        self.assertEqual(bad, [])

    def test_translations(self):
        bad = [f"{p.relative_to(REPO)}: {s}" for p in sources("*.po")
               for s in re.findall(r'^msg(?:id|str) "(.*)"$', p.read_text(encoding="utf-8"), re.M)
               if OLD.search(s)]
        self.assertEqual(bad, [])

    def test_desktop_names(self):
        bad = [f"{p.relative_to(REPO)}: {line}" for p in sources("*.desktop")
               for line in p.read_text(encoding="utf-8").splitlines()
               if re.match(r"(Name|GenericName|Comment)(\[[^]]+\])?=", line) and OLD.search(line)]
        self.assertEqual(bad, [])

    def test_package_descriptions(self):
        control = (REPO / "debian/control").read_text(encoding="utf-8")
        found = re.findall(r"^Description: .*(?:\n .*)*", control, re.M)
        self.assertTrue(found)
        self.assertEqual([d.splitlines()[0] for d in found if OLD.search(d)], [])

    def test_new_words_are_there(self):
        panel = (REPO / "controlpanel/po/fr/controlpanel.po").read_text(encoding="utf-8")
        self.assertIn('msgid "System Tools"\nmsgstr "Outils système"', panel)
        tasks = (REPO / "taskmanager/src/RunTaskSheet.qml").read_text(encoding="utf-8")
        self.assertIn('"Run this task as root"', tasks)


if __name__ == "__main__":
    sys.exit(not unittest.main(exit=False).result.wasSuccessful())
