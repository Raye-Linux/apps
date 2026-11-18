"""Every word the Clock shows has its French.

ki18n falls back on the English silently, so a forgotten string shows up
in English in the middle of a French window. The strings are read with
xgettext, from the QML and from the C++ (the ring tool's notifications
and the names of the cities).
"""
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
SRC = HERE.parent / "src"
PO = HERE.parent / "po" / "fr" / "clock.po"
KEYWORDS = ["--keyword=i18n:1", "--keyword=i18nc:1c,2", "--keyword=i18np:1,2",
            "--keyword=i18ncp:1c,2,3", "--keyword=ki18nc:1c,2", "--keyword=kli18nc:1c,2"]

ENTRY = re.compile(r'(?:msgctxt "((?:[^"\\]|\\.)*)"\n)?msgid "((?:[^"\\]|\\.)*)"\n(?:msgid_plural "(?:[^"\\]|\\.)*"\n)?'
                   r'(msgstr(?:\[0\])? "((?:[^"\\]|\\.)*)")')


def entries(text):
    return {(c, m): s for c, m, _, s in ENTRY.findall(text) if m}


class French(unittest.TestCase):
    def test_every_string_is_translated(self):
        xgettext = shutil.which("xgettext")
        self.assertTrue(xgettext, "xgettext is needed to read the strings")
        wanted = {}
        with tempfile.TemporaryDirectory() as tmp:
            for lang, files in (("JavaScript", sorted(SRC.glob("qml/*.qml"))), ("C++", sorted(SRC.glob("*.cpp")))):
                pot = Path(tmp) / (lang + ".pot")
                subprocess.run([xgettext, "--from-code=UTF-8", "-L", lang, *KEYWORDS, "--no-location", "--no-wrap",
                                "-o", str(pot)] + [str(f) for f in files], check=True)
                wanted.update(entries(pot.read_text()))
        # a template with almost nothing in it means xgettext read nothing
        self.assertGreater(len(wanted), 100)
        have = entries(PO.read_text())
        missing = [k for k in wanted if not have.get(k)]
        self.assertEqual(missing, [], "strings with no French")


if __name__ == "__main__":
    sys.exit(unittest.main())
