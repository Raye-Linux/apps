"""Every item, task and link of the Control Panel leads somewhere.

A page id that Settings does not have makes a link that opens Settings on
its home page, which reads as a broken Control Panel. The page ids come
from the settings repository next to this one when it is there (the image
build has both side by side), and from settings-pages.txt otherwise; when
both are there they must agree. Programs must be ones the image installs.
"""
import json
import os
import re
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
CATALOGUE = HERE.parent / "src" / "Catalogue.qml"
LAYOUT = Path(os.environ.get("RAYE_SETTINGS_LAYOUT", HERE.parents[2] / "settings" / "data" / "layout.json"))
SNAPSHOT = HERE / "settings-pages.txt"
CONTROL = HERE.parents[2] / "iso" / "packages" / "raye-base" / "debian" / "control"

# the package that brings each program the Control Panel starts
PROGRAMS = {
    "raye-settings": "raye-settings",
    "raye-taskmanager": "raye-taskmanager",
    "raye-regedit": "raye-regedit",
    "raye-terminal": "raye-terminal",
    "konsole": "konsole",
    "partitionmanager": "partitionmanager",
    "ksystemlog": "ksystemlog",
    "kcharselect": "kcharselect",
    "krdc": "krdc",
    "kwalletmanager": "kwalletmanager",
    "kwalletmanager5": "kwalletmanager",
}

# the classic items that have something to open on Raye
CLASSIC = {
    "admin", "autoplay", "backup", "credentials", "datetime", "defaults", "devices", "display",
    "encryption", "explorer", "firewall", "fonts", "access", "indexing", "internet", "keyboard",
    "mail", "mouse", "network", "personalization", "power", "printers", "programs", "recovery",
    "region", "security", "sound", "system", "taskbar", "troubleshoot", "users",
}


def layout_pages():
    layout = json.loads(LAYOUT.read_text())
    pages = set()
    for c in layout["categories"]:
        pages.add(c["id"])
        pages.update(p["id"] for p in c.get("pages", []))
    return pages


def snapshot_pages():
    return {l.strip() for l in SNAPSHOT.read_text().splitlines() if l.strip() and not l.startswith("#")}


def catalogue():
    return CATALOGUE.read_text()


def items():
    """id -> the text of its entry, from "{ id:" to the next one."""
    text = catalogue().split("readonly property var items:")[1].split("readonly property var tools:")[0]
    parts = re.split(r'\n        \{ id: "', text)[1:]
    return {p.split('"', 1)[0]: p for p in parts}


def desktop_depends():
    text = CONTROL.read_text()
    stanza = text.split("Package: raye-desktop", 1)[1].split("\nPackage:", 1)[0]
    depends = stanza.split("Depends:", 1)[1]
    # a staged line starts with "#": its package is not in the image yet
    lines = [l for l in depends.splitlines() if not l.lstrip().startswith("#")]
    return set(re.findall(r"[a-z0-9][a-z0-9.+-]+", " ".join(lines)))


class Catalogue(unittest.TestCase):
    def test_pages_exist_in_settings(self):
        pages = layout_pages() if LAYOUT.is_file() else snapshot_pages()
        wanted = set(re.findall(r'page: "([^"]+)"', catalogue()))
        wanted |= set(re.findall(r'"--page", "([^"]+)"', catalogue()))
        self.assertGreater(len(wanted), 50)
        self.assertEqual(sorted(wanted - pages), [], "pages Settings does not have")

    def test_snapshot_matches_settings(self):
        if LAYOUT.is_file():
            self.assertEqual(sorted(snapshot_pages() ^ layout_pages()), [], "settings-pages.txt is out of date")

    def test_programs_are_in_the_image(self):
        named = set(re.findall(r'programs?: "([^"]+)"', catalogue()))
        for group in re.findall(r'programs: \[([^\]]*)\]', catalogue()):
            named |= set(re.findall(r'"([^"]+)"', group))
        self.assertTrue(named)
        self.assertEqual(sorted(named - PROGRAMS.keys()), [], "programs with no package known here")
        if not CONTROL.is_file():
            self.skipTest("no iso repository next to this one")
        shipped = desktop_depends()
        # Raye's own terminal is not written yet; konsole stands in for it
        missing = sorted(p for p in named if PROGRAMS[p] not in shipped and p != "raye-terminal")
        self.assertEqual(missing, [], "programs the image does not install")

    def test_categories_are_full(self):
        text = catalogue()
        categories = re.findall(r'\{ id: "([^"]+)", icon:', text)
        self.assertEqual(len(categories), 8)
        found = items()
        for cat in categories:
            inside = [i for i, block in found.items()
                      if re.search(r'category: "%s"' % cat, block) or re.search(r'also: \[[^\]]*"%s"' % cat, block)]
            self.assertGreaterEqual(len(inside), 2, cat)
        for block in found.values():
            for cat in re.findall(r'category: "([^"]+)"', block):
                self.assertIn(cat, categories)

    def test_classic_items_are_there(self):
        self.assertEqual(sorted(CLASSIC - items().keys()), [])
        self.assertGreater(len(items()), 40)

    def test_every_item_opens_something(self):
        for item, block in items().items():
            head = block.split("tasks:")[0]
            self.assertTrue(re.search(r'\b(page|panel|programs?):', head), item)
            self.assertIn('name: i18nc(', block, item)
            self.assertIn('summary: i18nc(', block, item)

    def test_tasks_open_something(self):
        tasks = re.findall(r'\{ text: i18nc\([^)]*\), ([a-z]+):', catalogue())
        self.assertGreater(len(tasks), 60)
        self.assertEqual(sorted(set(tasks) - {"page", "panel", "program", "programs"}), [])

    def test_names_are_unique(self):
        names = re.findall(r'name: i18nc\("@title Control Panel item", "([^"]+)"\)', catalogue())
        self.assertEqual(len(names), len(set(names)))

    def test_no_raw_kde_module(self):
        # an item opens a Raye page or a program, never kcmshell
        self.assertNotIn("kcmshell", catalogue())
        self.assertNotIn("systemsettings", catalogue())


if __name__ == "__main__":
    sys.exit(unittest.main())
