#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Raye Linux
# SPDX-License-Identifier: GPL-3.0-or-later
"""The root password is asked for every change, Undo included."""

import pathlib
import sys
import xml.etree.ElementTree as ET

POLICY = pathlib.Path(__file__).resolve().parent.parent / "src" / "org.raye.regedit.policy"


def main():
    root = ET.parse(POLICY).getroot()
    bad = []
    for action in root.iter("action"):
        defaults = action.find("defaults")
        for name in ("allow_any", "allow_inactive", "allow_active"):
            value = defaults.findtext(name, "").strip()
            if value.endswith("_keep") or value == "yes":
                bad.append(f"{action.get('id')} {name}={value}")
    if bad:
        print("polkit keeps the password for these:", *bad, sep="\n  ")
        return 1
    print("ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
