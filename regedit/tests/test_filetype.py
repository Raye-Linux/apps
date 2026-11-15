#!/usr/bin/env python3
"""A .rayereg file opens in the Registry Editor, which asks before it
imports anything."""
import re
import sys
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "src"
TYPE = "application/x-raye-registry"


class FileType(unittest.TestCase):
    def test_mime_type_is_shipped(self):
        xml = SRC / "org.raye.regedit.mime.xml"
        ns = {"m": "http://www.freedesktop.org/standards/shared-mime-info"}
        root = ET.parse(xml).getroot()
        types = {t.get("type"): t for t in root.findall("m:mime-type", ns)}
        self.assertIn(TYPE, types)
        self.assertIn("*.rayereg", [g.get("pattern") for g in types[TYPE].findall("m:glob", ns)])
        self.assertIn("org.raye.regedit.mime.xml", (SRC / "CMakeLists.txt").read_text())

    def test_desktop_entry_takes_the_file(self):
        entry = (SRC / "org.raye.regedit.desktop").read_text()
        self.assertRegex(entry, rf"(?m)^MimeType=.*{re.escape(TYPE)};")
        self.assertRegex(entry, r"(?m)^Exec=raye-regedit %f$")

    def test_a_file_on_the_command_line_asks_first(self):
        main = (SRC / "main.cpp").read_text()
        qml = (SRC / "Main.qml").read_text()
        self.assertIn("askImport", main)
        self.assertRegex(qml, r"function askImport\(")


if __name__ == "__main__":
    sys.exit(unittest.main())
