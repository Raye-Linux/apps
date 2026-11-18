#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Raye Linux
# SPDX-License-Identifier: GPL-3.0-or-later
"""The policy, the two services and the Start entry agree with the code:
one helper path, and the page arguments main.cpp knows."""

import pathlib
import re
import sys
import unittest
import xml.etree.ElementTree as ET

SRC = pathlib.Path(__file__).resolve().parent.parent / "src"
HELPER = "/usr/libexec/raye-graphics-helper"


class Files(unittest.TestCase):
    def test_policy_names_the_helper(self):
        root = ET.parse(SRC / "org.raye.graphics.policy").getroot()
        actions = list(root.iter("action"))
        self.assertEqual([a.get("id") for a in actions], ["org.raye.graphics.apply"])
        paths = [(n.text or "").strip() for n in actions[0].iter("annotate")
                 if n.get("key") == "org.freedesktop.policykit.exec.path"]
        self.assertEqual(paths, [HELPER])
        self.assertEqual(actions[0].find("defaults").findtext("allow_any").strip(), "no")

    def test_services_run_the_helper(self):
        boot = (SRC / "raye-graphics.service").read_text()
        fan = (SRC / "raye-graphics-fan.service").read_text()
        self.assertIn(f"ExecStart={HELPER} boot\n", boot)
        self.assertIn(f"ExecStart={HELPER} fan\n", fan)
        # the fan goes back to the card whatever way the loop ends
        self.assertIn(f"ExecStopPost={HELPER} fan-auto\n", fan)

    def test_install_path_is_the_one_named(self):
        cmake = (SRC / "CMakeLists.txt").read_text()
        self.assertIn("install(TARGETS raye-graphics-helper DESTINATION libexec)", cmake)

    def test_start_entry_pages_exist(self):
        desktop = (SRC / "org.raye.graphics.desktop").read_text()
        main = (SRC / "main.cpp").read_text()
        args = re.findall(r"^Exec=raye-graphics (--\S+)$", desktop, re.M)
        self.assertEqual(sorted(args), ["--fan", "--performance"])
        for arg in args:
            self.assertIn(f'QStringLiteral("{arg}")', main)


if __name__ == "__main__":
    sys.exit(not unittest.main(exit=False).result.wasSuccessful())
