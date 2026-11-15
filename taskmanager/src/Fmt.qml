// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// How the figures read: sizes, rates and durations, in the user's
// language. Decimal units for disks and links, binary for memory, like
// Windows (which calls both "GB").

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
        // from 1000 KB on, as Windows does: never "1,000 KB"
        if (v >= 1024000)
            return i18nc("@info memory size in mebibytes", "%1 MB", number(v / 1048576, 1))
        if (v >= 1024)
            return i18nc("@info memory size in kibibytes", "%1 KB", number(v / 1024, 0))
        return i18nc("@info a size in bytes", "%1 B", number(v, 0))
    }

    function capacity(value) {
        const v = Number(value)
        if (v >= 1e12)
            return i18nc("@info disk size in terabytes", "%1 TB", number(v / 1e12, 1))
        return i18nc("@info disk size in gigabytes", "%1 GB", number(v / 1e9, 0))
    }

    function rate(bytesPerSecond) {
        const v = Number(bytesPerSecond)
        if (v >= 1048576)
            return i18nc("@info a transfer rate", "%1 MB/s", number(v / 1048576, 1))
        return i18nc("@info a transfer rate", "%1 KB/s", number(v / 1024, 0))
    }

    function bits(bitsPerSecond) {
        const v = Number(bitsPerSecond)
        if (v >= 1e9)
            return i18nc("@info a network speed", "%1 Gbps", number(v / 1e9, 1))
        if (v >= 1e6)
            return i18nc("@info a network speed", "%1 Mbps", number(v / 1e6, 1))
        return i18nc("@info a network speed", "%1 Kbps", number(v / 1e3, 0))
    }

    function percent(value) {
        return i18nc("@info a share, as a percentage", "%1%", number(value, 0))
    }

    function ghz(mhz) {
        return i18nc("@info a clock speed", "%1 GHz", number(mhz / 1000, 2))
    }

    // d:hh:mm:ss, as Windows shows the up time
    function uptime(seconds) {
        const s = Math.floor(seconds)
        const d = Math.floor(s / 86400)
        const two = n => (n < 10 ? "0" : "") + n
        return d + ":" + two(Math.floor(s % 86400 / 3600)) + ":" + two(Math.floor(s % 3600 / 60)) + ":" + two(s % 60)
    }
}
