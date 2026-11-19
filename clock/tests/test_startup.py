"""Clock opens without the media libraries.

QtMultimedia brings its player, FFmpeg and PulseAudio with it, about 40 MB.
Only the alarm sheet's Play button needs them, so the one file that imports
QtMultimedia is made from its file name when Play is pressed, never named as
a type the window would load at start.
"""
import re
import unittest
from pathlib import Path

QML = Path(__file__).resolve().parent.parent / "src" / "qml"


class Startup(unittest.TestCase):
    def test_only_the_preview_imports_multimedia(self):
        users = sorted(f.name for f in QML.glob("*.qml") if re.search(r"^import QtMultimedia\b", f.read_text(), re.M))
        self.assertEqual(users, ["SoundPreview.qml"])

    def test_preview_is_made_on_demand(self):
        for f in QML.glob("*.qml"):
            text = f.read_text()
            self.assertNotRegex(text, r"\bSoundPreview\s*\{", f.name)
        sheet = (QML / "AlarmSheet.qml").read_text()
        self.assertIn('Qt.createComponent("SoundPreview.qml")', sheet)


if __name__ == "__main__":
    unittest.main()
