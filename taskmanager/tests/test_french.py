"""Every word the Task Manager shows has its French.

ki18n falls back on the English silently, so a forgotten string shows up
in English in the middle of a French window and nothing else says so.
The strings are read with xgettext, the tool that makes the template.
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
PO = HERE.parent / "po" / "fr" / "taskmanager.po"

ENTRY = re.compile(r'(?:msgctxt "((?:[^"\\]|\\.)*)"\n)?msgid "((?:[^"\\]|\\.)*)"\n(?:msgid_plural "(?:[^"\\]|\\.)*"\n)?'
                   r'(msgstr(?:\[0\])? "((?:[^"\\]|\\.)*)")')


def entries(text):
    return {(c, m): s for c, m, _, s in ENTRY.findall(text) if m}


class French(unittest.TestCase):
    def test_every_string_is_translated(self):
        xgettext = shutil.which("xgettext")
        self.assertTrue(xgettext, "xgettext is needed to read the strings")
        sources = sorted(str(p) for p in SRC.glob("*.qml")) + [str(SRC / "main.cpp")]
        with tempfile.TemporaryDirectory() as tmp:
            pot = Path(tmp) / "t.pot"
            subprocess.run([xgettext, "--from-code=UTF-8", "-L", "JavaScript", "--keyword=i18n:1",
                            "--keyword=i18nc:1c,2", "--keyword=i18np:1,2", "--keyword=i18ncp:1c,2,3",
                            "--no-location", "--no-wrap", "-o", str(pot)] + sources, check=True)
            wanted = entries(pot.read_text())
        # a template with almost nothing in it means xgettext read nothing
        self.assertGreater(len(wanted), 150)
        have = entries(PO.read_text())
        missing = [k for k in wanted if not have.get(k)]
        self.assertEqual(missing, [], "strings with no French")


if __name__ == "__main__":
    sys.exit(unittest.main())
