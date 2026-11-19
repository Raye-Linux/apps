"""Only the page on screen is made, besides Processes.

The six other pages used to be built at start and kept for good, each with
its lists, graphs and models, though most people never leave Processes.
Processes stays: it is the start page and keeps the groups opened and the
sort chosen.
"""
import re
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "src"
OTHERS = ("PerformancePage", "StartupPage", "UsersPage", "DetailsPage", "ServicesPage", "SettingsPage")


class Pages(unittest.TestCase):
    def setUp(self):
        self.main = (SRC / "Main.qml").read_text()
        m = re.search(r"StackLayout \{(.*)\n    RunTaskSheet", self.main, re.S)
        self.assertIsNotNone(m, "the pages' StackLayout")
        self.stack = m.group(1)

    def test_processes_page_is_kept(self):
        self.assertRegex(self.stack, r"\n {20}ProcessesPage \{")

    def test_other_pages_are_made_when_shown(self):
        loaders = re.findall(r"\n {20}Loader \{(.*?)\n {20}\}", self.stack, re.S)
        found = {}
        for body in loaders:
            page = re.search(r"\b(\w+Page) \{", body)
            active = re.search(r"\n {24}active: root\.page === (\d+)\s*\n", body)
            self.assertIsNotNone(page)
            self.assertIsNotNone(active, f"{page.group(1)} is not made on demand")
            found[page.group(1)] = int(active.group(1))
        self.assertEqual(sorted(found), sorted(OTHERS))
        for index, name in enumerate(OTHERS, start=1):
            self.assertEqual(found[name], index, name)

    def test_no_page_binds_a_property_to_itself(self):
        # inside a Loader's component, "performance: performance" names the
        # page's own property, not the model of the window
        for name, value in re.findall(r"^\s+(\w+): (\w+)\s*$", self.stack, re.M):
            self.assertNotEqual(name, value, f"{name}: {value}")

    def test_details_is_shown_before_it_is_told_the_process(self):
        handler = re.search(r"onDetailsRequested: \(pid\) => \{(.*?)\}\n", self.main, re.S).group(1)
        self.assertLess(handler.index("root.page = 4"), handler.index("reveal(pid)"))


if __name__ == "__main__":
    unittest.main()
