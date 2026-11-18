// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tuning.h"

#include <QRegularExpression>

#include <algorithm>
#include <cmath>

namespace Raye::Graphics
{

namespace
{
std::optional<int> number(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^-?\\d{1,6}$"));
    if (!re.match(text).hasMatch())
        return std::nullopt;
    return text.toInt();
}

std::optional<QList<CurvePoint>> parseCurve(const QString &text)
{
    QList<CurvePoint> out;
    if (text.isEmpty())
        return out;
    const QStringList pairs = text.split(QLatin1Char(','));
    if (pairs.size() > 8)
        return std::nullopt;
    for (const QString &pair : pairs) {
        const QStringList parts = pair.split(QLatin1Char(':'));
        if (parts.size() != 2)
            return std::nullopt;
        const auto t = number(parts.at(0));
        const auto s = number(parts.at(1));
        if (!t || !s)
            return std::nullopt;
        out.append({*t, *s});
    }
    return out;
}
} // namespace

std::optional<Config> parseConfig(const QByteArray &text, QString *error)
{
    auto fail = [error](const QString &why) -> std::optional<Config> {
        if (error)
            *error = why;
        return std::nullopt;
    };
    if (text.size() > MaxConfigSize)
        return fail(QStringLiteral("too long"));
    Config c;
    QStringList seen;
    for (const QByteArray &raw : text.split('\n')) {
        const QString line = QString::fromUtf8(raw).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const qsizetype eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            return fail(QStringLiteral("not a key=value line: %1").arg(line.left(40)));
        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();
        if (seen.contains(key))
            return fail(QStringLiteral("%1 given twice").arg(key));
        seen.append(key);
        if (key == QLatin1String("performance")) {
            if (!performanceLevels().contains(value))
                return fail(QStringLiteral("unknown performance level"));
            c.performance = value;
        } else if (key == QLatin1String("profile") || key == QLatin1String("power-cap")) {
            const auto n = number(value);
            if (!n)
                return fail(QStringLiteral("%1 is not a number").arg(key));
            (key == QLatin1String("profile") ? c.profile : c.powerCapW) = *n;
        } else if (key == QLatin1String("fan")) {
            if (value != QLatin1String("auto") && value != QLatin1String("curve"))
                return fail(QStringLiteral("fan is auto or curve"));
            c.fanCurve = value == QLatin1String("curve");
        } else if (key == QLatin1String("curve")) {
            const auto curve = parseCurve(value);
            if (!curve)
                return fail(QStringLiteral("bad curve"));
            c.curve = *curve;
        } else {
            return fail(QStringLiteral("unknown key %1").arg(key.left(40)));
        }
    }
    return c;
}

QByteArray writeConfig(const Config &c)
{
    QByteArray out = "# Graphics card settings, written by Raye's Graphics Card panel\n";
    out += "performance=" + c.performance.toLatin1() + '\n';
    out += "profile=" + QByteArray::number(c.profile) + '\n';
    out += "power-cap=" + QByteArray::number(c.powerCapW) + '\n';
    out += QByteArray("fan=") + (c.fanCurve ? "curve" : "auto") + '\n';
    QByteArrayList points;
    for (const CurvePoint &p : c.curve)
        points.append(QByteArray::number(p.temp) + ':' + QByteArray::number(p.speed));
    out += "curve=" + points.join(',') + '\n';
    return out;
}

QString checkConfig(const Config &c, const Limits &l)
{
    if (!performanceLevels().contains(c.performance))
        return QStringLiteral("unknown performance level");
    if (c.performance != QLatin1String("auto") && l.level.isEmpty())
        return QStringLiteral("this card has no performance level");
    if (c.profile >= 0) {
        if (l.level.isEmpty())
            return QStringLiteral("this card has no performance level");
        const auto it = std::find_if(l.profiles.cbegin(), l.profiles.cend(), [&](const PowerProfile &p) {
            return p.index == c.profile;
        });
        if (it == l.profiles.cend())
            return QStringLiteral("no such profile");
        // CUSTOM wants a table of heuristics the panel does not offer
        if (it->name == QLatin1String("CUSTOM"))
            return QStringLiteral("the custom profile is not offered");
    } else if (c.profile != -1) {
        return QStringLiteral("no such profile");
    }
    if (c.powerCapW != 0) {
        if (!l.hasPowerCap)
            return QStringLiteral("this card has no power limit");
        if (c.powerCapW < l.capMinW || c.powerCapW > l.capMaxW)
            return QStringLiteral("power limit out of range");
    }
    if (!c.curve.isEmpty() || c.fanCurve) {
        if (c.fanCurve && !l.canCurve())
            return QStringLiteral("this card's fan cannot follow a curve");
        const int want = l.odCurve.valid ? l.curvePoints() : -1;
        if (want > 0 ? c.curve.size() != want : (c.curve.size() < 2 || c.curve.size() > 8))
            return QStringLiteral("wrong number of curve points");
        for (qsizetype i = 0; i < c.curve.size(); ++i) {
            const CurvePoint &p = c.curve.at(i);
            if (p.temp < l.curveTempMin() || p.temp > l.curveTempMax())
                return QStringLiteral("curve temperature out of range");
            if (p.speed < l.curveSpeedMin() || p.speed > l.curveSpeedMax())
                return QStringLiteral("curve speed out of range");
            if (i > 0 && (p.temp <= c.curve.at(i - 1).temp || p.speed < c.curve.at(i - 1).speed))
                return QStringLiteral("the curve must rise with the temperature");
        }
    }
    return {};
}

QList<CurvePoint> defaultCurve(const Limits &l)
{
    if (l.odCurve.valid) {
        // the firmware's own points, unless they are all zero (unset)
        bool set = false;
        for (const CurvePoint &p : l.odCurve.points)
            set = set || p.temp > 0;
        if (set && checkConfig({QStringLiteral("auto"), -1, 0, true, l.odCurve.points}, l).isEmpty())
            return l.odCurve.points;
    }
    const int lo = l.curveTempMin();
    const int hi = l.curveTempMax();
    const int smin = l.curveSpeedMin();
    const int n = l.curvePoints();
    // quiet while cool, flat out a little before the top
    const int from = std::max(lo, 40);
    const int top = std::max(from + n, hi - 5);
    QList<CurvePoint> out;
    for (int i = 0; i < n; ++i) {
        const double x = double(i) / (n - 1);
        out.append({from + int(std::lround((top - from) * x)), std::max(smin, int(std::lround(25 + 75 * x * x)))});
    }
    return out;
}

namespace
{
QString file(const Card &card, const char *name)
{
    return card.device + QLatin1Char('/') + QLatin1String(name);
}
QString hwmon(const Card &card, const char *name)
{
    return card.hwmon + QLatin1Char('/') + QLatin1String(name);
}

void planLevel(QList<Write> &out, const Card &card, const Limits &l, const QString &level, int profile)
{
    if (l.level.isEmpty())
        return;
    const QString path = file(card, "power_dpm_force_performance_level");
    int active = -1;
    for (const PowerProfile &p : l.profiles)
        if (p.active)
            active = p.index;
    const int target = profile >= 0 ? profile : l.defaultProfile;
    bool manual = false;
    // amdgpu takes a workload profile only in manual mode
    if (!l.profiles.isEmpty() && target >= 0 && target != active) {
        out.append({path, "manual"});
        out.append({file(card, "pp_power_profile_mode"), QByteArray::number(target)});
        manual = true;
    }
    const QByteArray want = profile >= 0 ? QByteArray("manual") : level.toLatin1();
    if (want != (manual ? QByteArray("manual") : l.level.toLatin1()))
        out.append({path, want});
}

void planOdCurve(QList<Write> &out, const Card &card, const QList<CurvePoint> &curve)
{
    const QString path = file(card, "gpu_od/fan_ctrl/fan_curve");
    for (qsizetype i = 0; i < curve.size(); ++i)
        out.append({path, QByteArray::number(i) + ' ' + QByteArray::number(curve.at(i).temp) + ' '
                              + QByteArray::number(curve.at(i).speed)});
    out.append({path, "c"});
}
} // namespace

QList<Write> planApply(const Card &card, const Limits &l, const Config &c)
{
    QList<Write> out;
    planLevel(out, card, l, c.performance, c.profile);

    if (l.hasPowerCap) {
        int cap = c.powerCapW > 0 ? c.powerCapW : l.capDefaultW;
        if (cap > 0 && cap != l.capW)
            out.append({hwmon(card, "power1_cap"), QByteArray::number(qint64(cap) * 1000000)});
    }

    if (l.odCurve.valid) {
        if (c.fanCurve) {
            planOdCurve(out, card, c.curve);
        } else {
            out.append({file(card, "gpu_od/fan_ctrl/fan_curve"), "r"});
            out.append({file(card, "gpu_od/fan_ctrl/fan_curve"), "c"});
        }
    } else if (l.pwmControl && !c.fanCurve && l.pwmMode != 2) {
        out.append({hwmon(card, "pwm1_enable"), "2"});
    }
    return out;
}

QList<Write> planReset(const Card &card, const Limits &l, int capW)
{
    QList<Write> out;
    planLevel(out, card, l, QStringLiteral("auto"), -1);
    if (l.hasPowerCap) {
        const int cap = l.capDefaultW > 0 ? l.capDefaultW : capW;
        if (cap > 0 && cap != l.capW)
            out.append({hwmon(card, "power1_cap"), QByteArray::number(qint64(cap) * 1000000)});
    }
    if (l.odCurve.valid) {
        out.append({file(card, "gpu_od/fan_ctrl/fan_curve"), "r"});
        out.append({file(card, "gpu_od/fan_ctrl/fan_curve"), "c"});
    } else if (l.pwmControl && l.pwmMode != 2) {
        out.append({hwmon(card, "pwm1_enable"), "2"});
    }
    return out;
}

bool needsFanService(const Limits &l, const Config &c)
{
    return c.fanCurve && !l.odCurve.valid && l.pwmControl;
}

} // namespace Raye::Graphics
