"""The Processes columns are wide enough to hold what they show.

A column once took Tokens.sizePillWidth, the 3 px of the accent pill, so
the figures were drawn on top of each other. The headings and the rows
must also use the same widths, or the figures end up under the wrong
heading.
"""
import re
import sys
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "src"


def widths(qml):
    return re.findall(r"(?:Layout\.preferredWidth|width):\s*([^\n]+)", (SRC / qml).read_text())


class Columns(unittest.TestCase):
    def test_no_column_takes_the_pill_width(self):
        for qml in ("GroupRow.qml", "ProcessesPage.qml"):
            for w in widths(qml):
                self.assertNotIn("sizePillWidth", w, f"{qml}: a column of 3 px")

    def test_rows_and_headings_share_one_width(self):
        page = (SRC / "ProcessesPage.qml").read_text()
        row = (SRC / "GroupRow.qml").read_text()
        # the headings: one status column and three figures
        self.assertEqual(page.count("Layout.preferredWidth: page.statusWidth"), 1)
        self.assertEqual(page.count("Layout.preferredWidth: page.figureWidth"), 3)
        # the rows are handed the same two widths, and use them
        self.assertIn("statusWidth: page.statusWidth", page)
        self.assertIn("figureWidth: page.figureWidth", page)
        self.assertEqual(row.count("width: row.figureWidth"), 3)
        self.assertIn("Layout.preferredWidth: row.statusWidth", row)
        m = re.search(r"readonly property int figureWidth:\s*(\d+)", page)
        self.assertTrue(m, "ProcessesPage.qml sets the width once")
        # "1,234.5 MB" in Inter 10 pt is about 75 px
        self.assertGreaterEqual(int(m.group(1)), 88)


if __name__ == "__main__":
    sys.exit(unittest.main())
