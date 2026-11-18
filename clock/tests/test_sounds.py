"""The alarm sounds come out as short, clean loops.

    python3 test_sounds.py path/to/make-sounds.py
"""
import array
import importlib.util
import sys
import tempfile
import unittest
import wave
from pathlib import Path

SCRIPT = Path(sys.argv.pop(1) if len(sys.argv) > 1 else
              Path(__file__).resolve().parent.parent / "sounds" / "make-sounds.py")


def load():
    spec = importlib.util.spec_from_file_location("make_sounds", SCRIPT)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class Sounds(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mod = load()
        cls.tmp = tempfile.TemporaryDirectory()
        assert cls.mod.main(["make-sounds.py", cls.tmp.name]) == 0

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def samples(self, name):
        with wave.open(str(Path(self.tmp.name) / (name + ".wav"))) as w:
            self.assertEqual((w.getnchannels(), w.getsampwidth()), (1, 2))
            rate = w.getframerate()
            data = array.array("h", w.readframes(w.getnframes()))
        return rate, data

    def test_the_five_sounds(self):
        self.assertEqual(list(self.mod.SOUNDS), ["glow", "chimes", "marimba", "pulse", "dawn"])

    def test_loud_enough_never_clipped(self):
        for name in self.mod.SOUNDS:
            _, data = self.samples(name)
            peak = max(abs(v) for v in data) / 32767
            self.assertGreater(peak, 0.6, name)
            self.assertLess(peak, 0.75, name)

    def test_short_loops_silent_at_the_seam(self):
        for name in self.mod.SOUNDS:
            rate, data = self.samples(name)
            self.assertTrue(0.8 < len(data) / rate < 4.0, name)
            # no click when the loop goes round
            self.assertLess(abs(data[0]), 200, name)
            self.assertLess(abs(data[-1]), 200, name)

    def test_same_every_time(self):
        a = self.samples("pulse")[1]
        with tempfile.TemporaryDirectory() as tmp:
            self.mod.main(["make-sounds.py", tmp, "pulse"])
            with wave.open(str(Path(tmp) / "pulse.wav")) as w:
                b = array.array("h", w.readframes(w.getnframes()))
        self.assertEqual(a, b)


if __name__ == "__main__":
    sys.exit(unittest.main())
