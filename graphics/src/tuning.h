// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The settings a user can give a card, the file they are kept in, the
// checks they pass, and the sysfs writes that apply them.
//
// The window and the root helper share all of it. The helper never trusts
// what it is given: it reads the card's limits again and runs checkConfig
// before a single write.

#pragma once

#include "amdgpu.h"

#include <QByteArray>
#include <QList>
#include <QString>

#include <optional>

namespace Raye::Graphics
{

struct Config {
    QString performance = QStringLiteral("auto"); // auto, low or high
    int profile = -1;                             // pp_power_profile_mode index, -1 for the card's default
    int powerCapW = 0;                            // 0 for the card's default
    bool fanCurve = false;                        // false: the card drives its fan itself
    QList<CurvePoint> curve;

    bool isDefault() const { return performance == QLatin1String("auto") && profile < 0 && powerCapW == 0 && !fanCurve; }
    bool operator==(const Config &o) const = default;
};

// the files are a few lines; anything longer is not one of them
constexpr int MaxConfigSize = 4096;

// "key=value" lines; unknown keys and bad numbers are refused
std::optional<Config> parseConfig(const QByteArray &text, QString *error = nullptr);
QByteArray writeConfig(const Config &config);

// empty when the card takes these settings, otherwise why not
QString checkConfig(const Config &config, const Limits &limits);

// a curve the panel offers on a card that has none yet
QList<CurvePoint> defaultCurve(const Limits &limits);

struct Write {
    QString file;
    QByteArray value;
    bool operator==(const Write &o) const = default;
};

// the writes that apply a config, in order. The fan of a card without a
// firmware curve is not in it: the fan service follows that curve.
QList<Write> planApply(const Card &card, const Limits &limits, const Config &config);

// the writes that give the card back its own settings. capW is the
// limit the card had before Raye changed it, for cards that do not
// publish their default; 0 when unknown.
QList<Write> planReset(const Card &card, const Limits &limits, int capW);

// whether the fan service has to follow this config's curve itself
bool needsFanService(const Limits &limits, const Config &config);

} // namespace Raye::Graphics
