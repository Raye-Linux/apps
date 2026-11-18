// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The readings of an NVIDIA card, from nvidia-smi. The driver keeps them
// to itself: there is nothing in sysfs to read, and nothing here changes
// them. Changes go through the NVIDIA Control Panel.

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

#include <cmath>

namespace Raye::Graphics
{

struct NvidiaReading {
    QString name;
    QString driver;
    QString slot; // 0000:01:00.0, lower case, as sysfs writes it
    int busy = -1, memBusy = -1;     // %
    int sclk = -1, mclk = -1;        // MHz
    qint64 vramUsed = -1, vramTotal = -1; // bytes
    double temp = NAN;
    int fanPercent = -1;
    double powerW = NAN, powerLimitW = NAN;
};

// the arguments nvidia-smi is run with; parseNvidiaSmi reads its output
QStringList nvidiaSmiArguments();
QList<NvidiaReading> parseNvidiaSmi(const QByteArray &csv);

} // namespace Raye::Graphics
