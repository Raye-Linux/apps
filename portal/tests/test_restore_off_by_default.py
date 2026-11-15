# SPDX-License-Identifier: LGPL-2.1-or-later
"""Both screen share pickers start with "restore this choice" unchecked."""
import pathlib
import re
import sys

here = pathlib.Path(__file__).resolve().parent.parent
bad = []
for rel, ident in (("qml/SharePicker.qml", "rememberOption"), ("src/ScreenChooserDialog.qml", "allowRestoreItem")):
    text = (here / rel).read_text(encoding="utf-8")
    m = re.search(r"id:\s*%s\b(.*?)(?:text:|\n\s*\})" % ident, text, re.S)
    if not m:
        bad.append(f"{rel}: {ident} not found")
    elif not re.search(r"checked:\s*false\b", m.group(1)):
        bad.append(f"{rel}: {ident} does not start unchecked")
if bad:
    print("\n".join(bad))
    sys.exit(1)
print("ok")
