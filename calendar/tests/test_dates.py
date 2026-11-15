#!/usr/bin/env python3
"""A date with both a day and a month is written the way the language
writes it: the pattern goes through i18nc, so English says "Tuesday,
November 11" and French "mardi 11 novembre". A pattern written out in the
code would put the day first for everyone.
"""
import re
import sys
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "src"
LITERAL = re.compile(r'(?:toLocaleDateString\(Qt\.locale\(\),\s*|QLocale\(\)\.toString\([^,]+,\s*QStringLiteral\()"([^"]+)"')


class Dates(unittest.TestCase):
    def test_day_and_month_patterns_are_translated(self):
        bad = []
        for path in sorted(SRC.rglob("*")):
            if path.suffix not in (".qml", ".cpp"):
                continue
            for pattern in LITERAL.findall(path.read_text()):
                if re.search(r"\bd\b", pattern) and "M" in pattern:
                    bad.append(f"{path.name}: {pattern}")
        self.assertEqual(bad, [])

    def test_english_reminders_say_am(self):
        text = (SRC / "qml" / "EventSheet.qml").read_text()
        self.assertEqual(re.findall(r'"[^"]*at 9:00"', text), [])


if __name__ == "__main__":
    sys.exit(unittest.main())
