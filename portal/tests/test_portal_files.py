#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Raye Linux
# SPDX-License-Identifier: LGPL-2.1-or-later
"""The files that make xdg-desktop-portal, D-Bus, systemd and KWin find
and trust the backend agree with each other.

usage: test_portal_files.py <portal source dir> <data build dir> <libexec dir> <access 0|1> [stock kde-portals.conf]
"""
import configparser
import pathlib
import re
import sys
import unittest

SOURCE, BUILD, LIBEXEC, ACCESS = sys.argv[1:5]
STOCK = sys.argv[5] if len(sys.argv) > 5 else ""
SOURCE = pathlib.Path(SOURCE)
BUILD = pathlib.Path(BUILD)
BINARY = f"{LIBEXEC}/xdg-desktop-portal-raye"
BUS_NAME = "org.freedesktop.impl.portal.desktop.raye"
IFACE = "org.freedesktop.impl.portal."


def ini(path):
    parser = configparser.ConfigParser(interpolation=None, strict=True)
    parser.optionxform = str  # keys keep their case
    with open(path, encoding="utf-8") as f:
        parser.read_file(f)
    return parser


class PortalFiles(unittest.TestCase):
    def test_portal_file(self):
        portal = ini(BUILD / "raye.portal")["portal"]
        self.assertEqual(portal["DBusName"], BUS_NAME)
        interfaces = portal["Interfaces"].split(";")
        expected = [IFACE + "ScreenCast", IFACE + "RemoteDesktop", IFACE + "Clipboard"]
        if ACCESS == "1":
            expected.append(IFACE + "Access")
        self.assertEqual(interfaces, expected)
        # chosen by kde-portals.conf only: without it the desktop's
        # backend serves everything, as before
        self.assertNotIn("UseIn", portal)

    def test_dbus_service_and_unit(self):
        service = ini(BUILD / f"{BUS_NAME}.service")["D-BUS Service"]
        self.assertEqual(service["Name"], BUS_NAME)
        self.assertEqual(service["Exec"], BINARY)
        unit_name = service["SystemdService"]
        self.assertEqual(unit_name, "xdg-desktop-portal-raye.service")
        unit = ini(BUILD / unit_name)
        self.assertEqual(unit["Service"]["ExecStart"], BINARY)
        self.assertEqual(unit["Service"]["BusName"], BUS_NAME)
        self.assertEqual(unit["Unit"]["PartOf"], "graphical-session.target")

    def test_kwin_trusts_the_binary(self):
        # KWin hands the privileged protocols to the program whose
        # .desktop file Exec is that very program
        entry = ini(BUILD / f"{BUS_NAME}.desktop")["Desktop Entry"]
        self.assertEqual(entry["Type"], "Application")
        self.assertEqual(entry["Exec"], BINARY)
        self.assertEqual(entry["NoDisplay"], "true")
        wanted = {"org_kde_kwin_fake_input", "org_kde_plasma_window_management", "zkde_screencast_unstable_v1"}
        self.assertEqual(set(entry["X-KDE-Wayland-Interfaces"].split(",")), wanted)
        self.assertTrue(entry.get("Name[fr]"))

    def test_preferred_backends(self):
        preferred = ini(SOURCE / "config" / "kde-portals.conf")["preferred"]
        self.assertEqual(preferred["default"], "kde;gtk")
        for name, backends in {
            "ScreenCast": "raye;kde",
            "RemoteDesktop": "raye;kde",
            "Clipboard": "raye;kde",
            "Access": "raye;kde",
            "FileChooser": "raye-files;kde",
            "AppChooser": "raye-files;kde",
        }.items():
            self.assertEqual(preferred[IFACE + name], backends, name)
        # every Raye choice falls back to the desktop's backend
        for key, value in preferred.items():
            choices = value.split(";")
            if choices[0].startswith("raye"):
                self.assertEqual(choices[-1], "kde", key)
        # the clipboard works on the remote desktop session: same backend
        self.assertEqual(preferred[IFACE + "Clipboard"], preferred[IFACE + "RemoteDesktop"])

    def test_keeps_the_stock_lines(self):
        # this file replaces the one Plasma installs: its lines must stay
        ours = ini(SOURCE / "config" / "kde-portals.conf")["preferred"]
        self.assertEqual(ours[IFACE + "Secret"], "kwallet")
        self.assertEqual(ours[IFACE + "Settings"], "kde;gtk")
        if not STOCK or not pathlib.Path(STOCK).is_file():
            self.skipTest("no stock kde-portals.conf on this machine")
        stock = ini(STOCK)["preferred"]
        for key in stock:
            self.assertIn(key, ours, f"{key} of the stock file is dropped")

    def test_notifications(self):
        rc = ini(SOURCE / "data" / "xdg-desktop-portal-raye.notifyrc")
        used = set()
        for path in (SOURCE / "src").rglob("*.cpp"):
            used |= set(re.findall(r'new KNotification\(QStringLiteral\("([a-z]+)"\)', path.read_text(encoding="utf-8")))
        self.assertTrue(used)
        for event in used:
            self.assertIn(f"Event/{event}", rc, event)
        for section in rc.sections():
            for key in ("Name", "Comment"):
                if key in rc[section]:
                    self.assertTrue(rc[section].get(f"{key}[fr]"), f"{section} {key}[fr]")

    def test_no_desktop_name_in_visible_words(self):
        visible = []
        rc = ini(SOURCE / "data" / "xdg-desktop-portal-raye.notifyrc")
        entry = ini(BUILD / f"{BUS_NAME}.desktop")
        for parser in (rc, entry):
            for section in parser.sections():
                for key, value in parser[section].items():
                    if key.startswith(("Name", "Comment")):
                        visible.append(value)
        for text in visible:
            self.assertNotRegex(text, r"\b(KDE|Plasma|Linux)\b", text)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]], verbosity=2)
