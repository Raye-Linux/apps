"""The Performance page updates its tiles and figures in place.

A Repeater given a list that is built again each second throws its
delegates away and makes them again. A new graph has nothing to show until
its first paint, one frame later, so the disk, network and GPU tiles went
blank for a frame at every reading.
"""
import re
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "src"


class Performance(unittest.TestCase):
    def setUp(self):
        self.page = (SRC / "PerformancePage.qml").read_text()
        self.models = re.findall(r"^\s*model:\s*(.+?)\s*$", self.page, re.M)

    def test_repeaters_take_a_model_or_a_count(self):
        self.assertTrue(self.models)
        for model in self.models:
            ok = re.fullmatch(r"page\.performance\.\w+List", model) or re.search(r"\.length(\s*\*\s*\d+)?$", model)
            self.assertTrue(ok, f"a Repeater rebuilt every reading: model: {model}")

    def test_device_tiles_use_the_list_models(self):
        for name in ("diskList", "adapterList", "gpuList"):
            self.assertIn(f"model: page.performance.{name}", self.page)
        header = (SRC / "performancemodel.h").read_text()
        for name in ("diskList", "adapterList", "gpuList"):
            self.assertRegex(header, rf"Q_PROPERTY\(QAbstractItemModel \*{name} READ \w+ CONSTANT\)")


if __name__ == "__main__":
    unittest.main()
