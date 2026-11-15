#!/usr/bin/env python3
"""The English the apps show is American: color, center, favorite.
Comments and identifiers may say what they like.

    python3 tests/test_american.py
"""
import pathlib
import re
import sys
import unittest

REPO = pathlib.Path(__file__).resolve().parent.parent
BRITISH = re.compile(r"colour|favourit|centre|optimis|cancell|grey|behaviour|licence|organis|recognis|"
                     r"customis|personalis|minimis|maximis|catalogue|programme\b|travell|honour", re.I)
CALL = re.compile(r'\bi18n[a-z]*\((?:"[^"]*",\s*)?"([^"]*)"')
DESKTOP = re.compile(r"^(?:Name|GenericName|Comment)=(.*)$", re.M)


def sources(*patterns):
    for pattern in patterns:
        for path in sorted(REPO.rglob(pattern)):
            parts = path.relative_to(REPO).parts
            if path.is_file() and not {".git", "debian", "obj-x86_64-linux-gnu", "build"} & set(parts) \
                    and "tests" not in parts:
                yield path


class American(unittest.TestCase):
    def test_i18n_strings(self):
        bad = [f"{p.relative_to(REPO)}: {s}" for p in sources("*.qml", "*.js", "*.cpp")
               for s in CALL.findall(p.read_text()) if BRITISH.search(s)]
        self.assertEqual(bad, [])

    def test_desktop_and_policy(self):
        bad = [f"{p.relative_to(REPO)}: {s}" for p in sources("*.desktop") for s in DESKTOP.findall(p.read_text())
               if BRITISH.search(s)]
        for p in sources("*.policy"):
            for s in re.findall(r"<(?:description|message)>([^<]*)<", p.read_text()):
                if BRITISH.search(s):
                    bad.append(f"{p.relative_to(REPO)}: {s}")
        self.assertEqual(bad, [])


if __name__ == "__main__":
    sys.exit(unittest.main())
