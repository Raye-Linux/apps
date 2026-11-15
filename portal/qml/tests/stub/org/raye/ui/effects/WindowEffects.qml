// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// Stands in for the C++ part of org.raye.ui (blur behind a window) when
// the tests use the module from the theme's source tree, where it is not
// built. Never installed.
import QtQuick

QtObject {
    property Window window
    property bool blurBehind: false
    readonly property bool transparencyEffects: false
    readonly property bool blurAvailable: false
    function refresh() {}
}
