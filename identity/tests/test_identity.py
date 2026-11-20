#!/usr/bin/env python3
# The table of app names and the generator that writes them next to the
# stock .desktop files. Runs without a session, on files in a temp folder.

import configparser
import json
import os
import pathlib
import re
import stat
import subprocess
import sys
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent
TOOL = ROOT / "raye-app-overrides"
TABLE = ROOT / "identity.json"

# Words nobody using Raye should read in the Start menu, search or Open with.
UPSTREAM = re.compile(
    r"\b(kde|plasma|wine|winetricks|konsole|kate|kcalc|okular|gwenview|haruna|elisa|ark|"
    r"spectacle|kolourpaint|kcharselect|krdc|skanpage|synaptic|btrfs|btop|htop|yakuake|"
    r"krecorder|kclock|kinfocenter|ksystemlog|kwallet\w*)\b", re.I)

KONSOLE = """[Desktop Entry]
Type=Application
Name=Konsole
Name[de]=Konsole
Name[fr]=Konsole
GenericName=Terminal
GenericName[fr]=Terminal
Comment=Command line access
Comment[fr]=Accès en ligne de commande
Keywords=shell;kde;
Keywords[fr]=shell;kde;
Exec=konsole --new-window
Icon=utilities-terminal
Categories=Qt;KDE;System;TerminalEmulator;
Actions=NewTab;
X-KDE-StartupNotify=true

[Desktop Action NewTab]
Name=Open a New Tab
Name[fr]=Ouvrir un onglet
Exec=konsole --new-tab
"""

SYNAPTIC = """[Desktop Entry]
Type=Application
Name=Synaptic Package Manager
Exec=synaptic-pkexec
Icon=synaptic
MimeType=application/vnd.debian.binary-package;
"""


def entry(path):
    cp = configparser.ConfigParser(interpolation=None, strict=False)
    cp.optionxform = str
    cp.read(path, encoding="utf-8")
    return cp


class Table(unittest.TestCase):
    def setUp(self):
        self.table = json.loads(TABLE.read_text(encoding="utf-8"))["apps"]

    def test_kdes_settings_leave_start_for_raye_settings(self):
        for desktop_id in ("systemsettings.desktop", "kdesystemsettings.desktop"):
            with self.subTest(desktop_id):
                # Raye Settings does their work; the program stays for the
                # pages of its own Settings opens
                self.assertTrue(self.table[desktop_id].get("hidden"))

    def test_synaptic_is_not_hidden_from_who_installs_it(self):
        # the image no longer has it; a person who installs it wants to see it
        self.assertNotIn("synaptic.desktop", self.table)

    def test_the_apps_the_owner_named_get_a_plain_name(self):
        for desktop_id, en, fr in (
            ("org.kde.konsole.desktop", "Terminal", "Terminal"),
            ("org.kde.kate.desktop", "Text Editor", "Éditeur de texte"),
            ("org.kde.kcalc.desktop", "Calculator", "Calculatrice"),
            ("org.kde.gwenview.desktop", "Photos", "Photos"),
            ("org.kde.haruna.desktop", "Media Player", "Lecteur multimédia"),
            ("org.kde.elisa.desktop", "Music", "Musique"),
            ("org.kde.okular.desktop", "Document Viewer", "Visionneuse de documents"),
            ("org.kde.ark.desktop", "Archive Manager", "Gestionnaire d'archives"),
            ("nvidia-settings.desktop", "NVIDIA Control Panel", "Panneau de configuration NVIDIA"),
        ):
            self.assertEqual(self.table[desktop_id]["name"], {"en": en, "fr": fr})

    def test_every_entry_either_hides_or_has_both_languages(self):
        for desktop_id, app in self.table.items():
            self.assertTrue(desktop_id.endswith(".desktop"), desktop_id)
            if "name" not in app:
                self.assertTrue(app.get("hidden"), f"{desktop_id}: no name and not hidden")
                continue
            for key in ("name", "comment"):
                self.assertEqual(sorted(app[key]), ["en", "fr"], f"{desktop_id} {key}")
                for text in app[key].values():
                    self.assertTrue(text.strip(), f"{desktop_id} {key}")
            self.assertTrue(app.get("icon"), f"{desktop_id}: no icon")

    def test_nothing_upstream_is_left_in_the_words_people_read(self):
        for desktop_id, app in self.table.items():
            for key in ("name", "comment"):
                for lang, text in app.get(key, {}).items():
                    self.assertIsNone(UPSTREAM.search(text), f"{desktop_id} {key} {lang}: {text}")

    def test_no_two_visible_apps_share_a_name(self):
        for lang in ("en", "fr"):
            seen = {}
            for desktop_id, app in self.table.items():
                if app.get("hidden") or "name" not in app:
                    continue
                name = app["name"][lang]
                self.assertNotIn(name, seen, f"{desktop_id} and {seen.get(name)}")
                seen[name] = desktop_id

    def test_kde_clock_gives_way_to_raye_clock(self):
        # both are called Clock: without this, Start lists two Clocks
        self.assertEqual(self.table["org.kde.kclock.desktop"].get("replaced_by"),
                         "org.raye.clock.desktop")


class Generator(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        base = pathlib.Path(self.tmp.name)
        self.source = base / "applications"
        self.output = base / "out" / "applications"
        self.source.mkdir()
        self.table = base / "table.json"
        self.table.write_text(json.dumps({"apps": {
            "org.kde.konsole.desktop": {
                "name": {"en": "Terminal", "fr": "Terminal"},
                "comment": {"en": "Run commands", "fr": "Lancer des commandes"},
                "keywords": {"en": "shell;console;", "fr": "invite;console;"},
                "icon": "utilities-terminal",
            },
            "synaptic.desktop": {"hidden": True},
            "org.kde.gone.desktop": {"hidden": True},
            "org.kde.plasma-systemmonitor.desktop": {
                "name": {"en": "Task Manager", "fr": "Gestionnaire des tâches"},
                "comment": {"en": "See what runs", "fr": "Voir ce qui tourne"},
                "icon": "utilities-system-monitor",
                "replaced_by": "org.raye.taskmanager.desktop",
            },
        }}), encoding="utf-8")

    def run_tool(self, check=True):
        return subprocess.run(
            [sys.executable, str(TOOL), "--source", str(self.source), "--table", str(self.table),
             "--output", str(self.output)],
            check=check, capture_output=True, text=True)

    def write(self, name, text):
        (self.source / name).write_text(text, encoding="utf-8")

    def test_name_and_icon_are_replaced_in_both_languages(self):
        self.write("org.kde.konsole.desktop", KONSOLE)
        self.run_tool()
        main = entry(self.output / "org.kde.konsole.desktop")["Desktop Entry"]
        self.assertEqual(main["Name"], "Terminal")
        self.assertEqual(main["Name[fr]"], "Terminal")
        self.assertEqual(main["Comment"], "Run commands")
        self.assertEqual(main["Comment[fr]"], "Lancer des commandes")
        self.assertEqual(main["Icon"], "utilities-terminal")
        self.assertEqual(main["Keywords"], "shell;console;konsole;")
        self.assertEqual(main["Keywords[fr]"], "invite;console;konsole;")

    def test_other_translations_of_the_old_name_go_away(self):
        self.write("org.kde.konsole.desktop", KONSOLE)
        self.run_tool()
        text = (self.output / "org.kde.konsole.desktop").read_text(encoding="utf-8")
        self.assertNotIn("Name[de]", text)
        self.assertNotIn("=Konsole", text)
        self.assertNotIn("GenericName", text)
        self.assertNotIn("kde;\n", text.replace("Qt;KDE;System", ""))

    def test_the_old_name_still_finds_the_app(self):
        # someone who types "konsole" wants the Terminal they have, not the
        # Store's offer of it: the old name stays as a search word only
        self.write("org.kde.konsole.desktop", KONSOLE)
        self.run_tool()
        main = entry(self.output / "org.kde.konsole.desktop")["Desktop Entry"]
        self.assertEqual(main["Name"], "Terminal")
        self.assertIn("konsole", main["Keywords"].split(";"))
        self.assertIn("konsole", main["Keywords[fr]"].split(";"))

    def test_an_app_without_words_of_its_own_still_gets_its_old_name(self):
        self.write("synaptic.desktop", SYNAPTIC)
        table = json.loads(self.table.read_text(encoding="utf-8"))
        table["apps"]["synaptic.desktop"] = {
            "name": {"en": "Packages", "fr": "Paquets"},
            "comment": {"en": "Add programs", "fr": "Ajouter des programmes"},
            "icon": "package",
        }
        self.table.write_text(json.dumps(table), encoding="utf-8")
        self.run_tool()
        main = entry(self.output / "synaptic.desktop")["Desktop Entry"]
        self.assertEqual(main["Keywords"], "synaptic package manager;")
        self.assertEqual(main["Keywords[fr]"], "synaptic package manager;")

    def test_what_starts_the_app_is_left_alone(self):
        self.write("org.kde.konsole.desktop", KONSOLE)
        self.run_tool()
        cp = entry(self.output / "org.kde.konsole.desktop")
        self.assertEqual(cp["Desktop Entry"]["Exec"], "konsole --new-window")
        self.assertEqual(cp["Desktop Entry"]["Categories"], "Qt;KDE;System;TerminalEmulator;")
        self.assertEqual(cp["Desktop Entry"]["X-KDE-StartupNotify"], "true")
        self.assertEqual(cp["Desktop Action NewTab"]["Exec"], "konsole --new-tab")
        self.assertEqual(cp["Desktop Action NewTab"]["Name[fr]"], "Ouvrir un onglet")

    def test_a_hidden_app_stays_for_open_with(self):
        self.write("synaptic.desktop", SYNAPTIC)
        self.run_tool()
        main = entry(self.output / "synaptic.desktop")["Desktop Entry"]
        self.assertEqual(main["NoDisplay"], "true")
        self.assertEqual(main["MimeType"], "application/vnd.debian.binary-package;")
        self.assertEqual(main["Name"], "Synaptic Package Manager")

    def test_an_app_taken_out_of_the_table_gets_its_stock_file_back(self):
        # Beta 2 stopped hiding Synaptic: a copy a person installs shows
        self.write("synaptic.desktop", SYNAPTIC)
        self.run_tool()
        self.assertTrue((self.output / "synaptic.desktop").is_file())
        table = json.loads(self.table.read_text(encoding="utf-8"))
        del table["apps"]["synaptic.desktop"]
        self.table.write_text(json.dumps(table), encoding="utf-8")
        (self.output / "notes.txt").write_text("not ours to judge\n", encoding="utf-8")
        self.run_tool()
        self.assertFalse((self.output / "synaptic.desktop").exists())
        self.assertTrue((self.output / "notes.txt").exists())

    def test_a_hidden_line_is_not_written_twice(self):
        self.write("synaptic.desktop", SYNAPTIC.replace("Type=Application\n", "Type=Application\nNoDisplay=false\n"))
        self.run_tool()
        text = (self.output / "synaptic.desktop").read_text(encoding="utf-8")
        self.assertEqual(text.count("NoDisplay="), 1)
        self.assertIn("NoDisplay=true", text)

    def test_an_app_that_is_not_installed_gets_no_file(self):
        self.run_tool()
        self.assertFalse((self.output / "org.kde.gone.desktop").exists())

    def test_a_removed_app_loses_its_file(self):
        self.write("synaptic.desktop", SYNAPTIC)
        self.run_tool()
        (self.source / "synaptic.desktop").unlink()
        self.run_tool()
        self.assertFalse((self.output / "synaptic.desktop").exists())

    def test_a_file_someone_else_put_there_stays(self):
        self.output.mkdir(parents=True)
        (self.output / "note.txt").write_text("mine")
        self.run_tool()
        self.assertTrue((self.output / "note.txt").exists())

    def test_an_app_raye_replaced_is_hidden_and_keeps_no_name_of_its_own(self):
        monitor = "[Desktop Entry]\nType=Application\nName=System Monitor\nExec=x\nIcon=y\n"
        self.write("org.kde.plasma-systemmonitor.desktop", monitor)
        self.run_tool()
        main = entry(self.output / "org.kde.plasma-systemmonitor.desktop")["Desktop Entry"]
        self.assertEqual(main["Name"], "Task Manager")
        self.assertNotIn("NoDisplay", main)
        self.write("org.raye.taskmanager.desktop", monitor)
        self.run_tool()
        main = entry(self.output / "org.kde.plasma-systemmonitor.desktop")["Desktop Entry"]
        self.assertEqual(main["NoDisplay"], "true")

    def test_one_broken_file_does_not_stop_the_others_and_fails_the_run(self):
        self.write("org.kde.konsole.desktop", "not a desktop file at all\n")
        self.write("synaptic.desktop", SYNAPTIC)
        done = self.run_tool(check=False)
        self.assertEqual(done.returncode, 1)
        self.assertIn("org.kde.konsole.desktop", done.stderr)
        self.assertTrue((self.output / "synaptic.desktop").exists())
        self.assertFalse((self.output / "org.kde.konsole.desktop").exists())

    def test_files_are_readable_by_everyone(self):
        self.write("synaptic.desktop", SYNAPTIC)
        old = os.umask(0o077)
        try:
            self.run_tool()
        finally:
            os.umask(old)
        mode = stat.S_IMODE((self.output / "synaptic.desktop").stat().st_mode)
        self.assertEqual(mode, 0o644)

    def test_running_it_again_changes_nothing(self):
        self.write("org.kde.konsole.desktop", KONSOLE)
        self.run_tool()
        first = (self.output / "org.kde.konsole.desktop").read_bytes()
        self.run_tool()
        self.assertEqual(first, (self.output / "org.kde.konsole.desktop").read_bytes())
        self.assertEqual(sorted(p.name for p in self.output.iterdir()), ["org.kde.konsole.desktop"])


class EnvScript(unittest.TestCase):
    def run_script(self, value):
        env = {"PATH": os.environ["PATH"]}
        if value is not None:
            env["XDG_DATA_DIRS"] = value
        done = subprocess.run(
            ["sh", "-c", f'. "{ROOT}/raye-app-overrides.sh"; printf %s "$XDG_DATA_DIRS"'],
            env=env, check=True, capture_output=True, text=True)
        return done.stdout

    def test_overrides_come_first(self):
        self.assertEqual(self.run_script("/a:/usr/share"), "/usr/share/raye/app-overrides:/a:/usr/share")

    def test_the_default_list_is_kept_when_nothing_is_set(self):
        self.assertEqual(self.run_script(None),
                         "/usr/share/raye/app-overrides:/usr/local/share:/usr/share")

    def test_it_is_not_added_twice(self):
        once = "/usr/share/raye/app-overrides:/usr/share"
        self.assertEqual(self.run_script(once), once)


if __name__ == "__main__":
    unittest.main()
