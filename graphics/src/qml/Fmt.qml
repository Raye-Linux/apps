// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// How the figures read, in the user's language, and the names of the
// card's workload profiles.

pragma Singleton

import QtQuick

QtObject {
    function number(value, decimals) {
        return Number(value).toLocaleString(Qt.locale(), "f", decimals)
    }
    function bytes(value) {
        const v = Number(value)
        if (v >= 1073741824)
            return i18nc("@info memory size in gibibytes", "%1 GB", number(v / 1073741824, 1))
        return i18nc("@info memory size in mebibytes", "%1 MB", number(v / 1048576, 0))
    }
    function percent(value) {
        return i18nc("@info a share, as a percentage", "%1%", number(value, 0))
    }
    function mhz(value) {
        return i18nc("@info a clock speed", "%1 MHz", number(value, 0))
    }
    function celsius(value) {
        return i18nc("@info a temperature", "%1 °C", number(value, 0))
    }
    function watts(value) {
        return i18nc("@info a power", "%1 W", number(value, 0))
    }
    function rpm(value) {
        return i18nc("@info a fan speed in turns a minute", "%1 RPM", number(value, 0))
    }

    // amdgpu's names, in words
    function profileName(name) {
        switch (name) {
        case "BOOTUP_DEFAULT": return i18nc("@item workload profile", "Default")
        case "3D_FULL_SCREEN": return i18nc("@item workload profile", "Games in full screen")
        case "WINDOW_3D": return i18nc("@item workload profile", "Games in a window")
        case "POWER_SAVING": return i18nc("@item workload profile", "Power saving")
        case "VIDEO": return i18nc("@item workload profile", "Video")
        case "VR": return i18nc("@item workload profile", "Virtual reality")
        case "COMPUTE": return i18nc("@item workload profile", "Compute")
        case "CAPPED": return i18nc("@item workload profile", "Capped")
        case "UNCAPPED": return i18nc("@item workload profile", "Uncapped")
        }
        const words = name.toLowerCase().replace(/_/g, " ")
        return words.charAt(0).toUpperCase() + words.slice(1)
    }
}
