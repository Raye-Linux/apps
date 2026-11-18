// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// What amdgpu says about a card in sysfs, and what it lets us change.
// Every reading may be missing: an integrated GPU has no fan and often no
// power limit, an older card no junction sensor. Missing is -1 (or NaN
// for a temperature or a power), and the window hides what is missing.

#pragma once

#include "cards.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <cmath>

namespace Raye::Graphics
{

struct DpmLevel {
    int level = -1; // -1 for the sleep level "S"
    int mhz = 0;
    bool active = false;
};
// pp_dpm_sclk, pp_dpm_mclk: "0: 300Mhz\n1: 1366Mhz *\n"
QList<DpmLevel> parseDpm(const QByteArray &text);

struct PowerProfile {
    int index = -1;
    QString name; // BOOTUP_DEFAULT, 3D_FULL_SCREEN, POWER_SAVING...
    bool active = false;
};
// pp_power_profile_mode. Older cards write one profile per row, newer
// ones (RDNA3) one per column of a header row; both are read here.
QList<PowerProfile> parseProfiles(const QByteArray &text);

struct CurvePoint {
    int temp = 0;  // °C
    int speed = 0; // % of the fan's full speed
    bool operator==(const CurvePoint &o) const { return temp == o.temp && speed == o.speed; }
};

struct OdFanCurve {
    QList<CurvePoint> points;
    int tempMin = 0, tempMax = 0, speedMin = 0, speedMax = 0;
    bool valid = false;
};
// gpu_od/fan_ctrl/fan_curve, the fan curve the card's own firmware
// follows (RDNA3 and later)
OdFanCurve parseOdFanCurve(const QByteArray &text);

struct Reading {
    int busy = -1;     // %
    int memBusy = -1;  // %
    qint64 vramUsed = -1, vramTotal = -1; // bytes
    qint64 gttUsed = -1, gttTotal = -1;
    int sclk = -1, mclk = -1; // MHz
    double edge = NAN, junction = NAN, memory = NAN; // °C
    int fanRpm = -1;
    int fanPercent = -1;
    double powerW = NAN;
};
Reading readReading(const Card &card);

// the performance levels this panel offers; amdgpu has more (manual and
// the profile_* ones), set only on the way to a workload profile
inline const QStringList &performanceLevels()
{
    static const QStringList levels{QStringLiteral("auto"), QStringLiteral("low"), QStringLiteral("high")};
    return levels;
}

struct Limits {
    QString level;                    // power_dpm_force_performance_level now, empty if none
    QList<PowerProfile> profiles;     // empty if the card has none
    int defaultProfile = -1;          // the index of BOOTUP_DEFAULT
    bool hasPowerCap = false;
    int capW = 0, capMinW = 0, capMaxW = 0;
    int capDefaultW = -1;             // -1 when the card does not say
    bool hasFan = false;              // a fan speed can be read
    int fanMinRpm = -1, fanMaxRpm = -1;
    bool pwmControl = false;          // pwm1 and pwm1_enable can be written
    int pwmMin = 0, pwmMax = 255;
    int pwmMode = -1;                 // pwm1_enable: 1 manual, 2 automatic
    OdFanCurve odCurve;               // valid on RDNA3 and later
    double tempCrit = NAN;            // °C, of the sensor the curve follows
    bool junctionSensor = false;      // the curve follows the junction (hottest) sensor

    bool canCurve() const { return odCurve.valid || pwmControl; }
    // the highest temperature a curve point may have; above it the fan
    // runs flat out whatever the curve says
    int curveTempMax() const;
    int curveTempMin() const { return odCurve.valid ? odCurve.tempMin : 20; }
    int curveSpeedMin() const { return odCurve.valid ? odCurve.speedMin : 0; }
    int curveSpeedMax() const { return odCurve.valid ? odCurve.speedMax : 100; }
    int curvePoints() const { return odCurve.valid ? int(odCurve.points.size()) : 5; }
};
Limits readLimits(const Card &card);

// the temperature a fan curve follows: the junction if there is one
double curveTemperature(const Reading &r);

} // namespace Raye::Graphics
