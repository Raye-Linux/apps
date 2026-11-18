// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The graphics cards of the PC, as /sys/class/drm lists them. sysRoot is
// "/sys" on a real machine; the tests and the screenshots give a folder
// of their own.

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace Raye::Graphics
{

enum class Vendor { Amd, Nvidia, Intel, Other };

struct Card {
    QString name;     // card0
    QString device;   // <sysRoot>/class/drm/card0/device
    QString hwmon;    // <device>/hwmon/hwmonN, empty when there is none
    QString slot;     // 0000:03:00.0, the same after a restart
    QString vendorId; // 1002
    QString deviceId; // 67df
    QString driver;   // amdgpu
    QString model;    // Radeon RX 580, or empty
    Vendor vendor = Vendor::Other;

    // amdgpu is the only driver whose files this panel changes
    bool tunable() const { return vendor == Vendor::Amd && driver == QLatin1String("amdgpu"); }
};

QList<Card> findCards(const QString &sysRoot, const QByteArray &pciIds = {});

// the card at a PCI slot, or one with an empty name
Card cardAt(const QString &sysRoot, const QString &slot);

// 0000:03:00.0 and nothing else: the helper takes it from the command line
bool isSlot(const QString &text);

// "Advanced Micro Devices, Inc. [AMD/ATI] Ellesmere [Radeon RX 470/480/570/570X/580/580X/590]"
// comes out as "Radeon RX 470/480/570/570X/580/580X/590"
QString modelName(const QByteArray &pciIds, const QString &vendor, const QString &device);

// small file readers, shared by the readings and the helper
QByteArray readFile(const QString &path, qint64 max = 65536);
QString readLine(const QString &path);
// a whole number, or fallback when the file is missing or not a number
qint64 readNumber(const QString &path, qint64 fallback = -1);

} // namespace Raye::Graphics
