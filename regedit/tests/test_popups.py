"""Sheets and menus of the Registry Editor have a solid fill.

Card and the bgCard / bgLayer colours are see-through, meant for a card on
the page. Behind a dialog the page then reads through its text.
"""
import re
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "src"
SEE_THROUGH = ("bgCard", "bgLayer", "bgCardHover")


def popups():
    for qml in sorted(SRC.glob("*.qml")):
        text = qml.read_text()
        if re.search(r"^QQC2\.(Dialog|Popup|Menu)\s*\{", text, re.M):
            yield qml.name, text


def background(text):
    m = re.search(r"^    background:\s*(\w+)\s*\{(.*?)^    \}", text, re.M | re.S)
    return (m.group(1), m.group(2)) if m else (None, "")


class Popups(unittest.TestCase):
    def test_found_the_sheets(self):
        names = [name for name, _ in popups()]
        for name in ("Sheet.qml", "ContextMenu.qml"):
            self.assertIn(name, names)

    def test_solid_background(self):
        for name, text in popups():
            kind, body = background(text)
            self.assertEqual(kind, "Rectangle", f"{name}: background is {kind}")
            color = re.search(r"^\s*color:\s*(.+)$", body, re.M)
            self.assertTrue(color, f"{name}: background has no colour")
            for token in SEE_THROUGH:
                self.assertNotRegex(color.group(1), rf"Tokens\.{token}\b", f"{name}: see-through background")

    def test_modal_sheets_dim_the_window(self):
        for name, text in popups():
            if re.search(r"^\s*modal:\s*true", text, re.M):
                self.assertTrue("QQC2.Overlay.modal:" in text, f"{name}: nothing dims the window behind it")


if __name__ == "__main__":
    unittest.main()
