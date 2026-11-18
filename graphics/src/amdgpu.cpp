// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "amdgpu.h"

#include <QFile>
#include <QRegularExpression>

#include <algorithm>
#include <climits>

namespace Raye::Graphics
{

QList<DpmLevel> parseDpm(const QByteArray &text)
{
    static const QRegularExpression re(QStringLiteral("^\\s*(\\d+|S):\\s*(\\d+)\\s*mhz\\s*(\\*)?"),
                                       QRegularExpression::CaseInsensitiveOption);
    QList<DpmLevel> out;
    for (const QByteArray &line : text.split('\n')) {
        const auto m = re.match(QString::fromLatin1(line));
        if (!m.hasMatch())
            continue;
        DpmLevel l;
        l.level = m.captured(1) == QLatin1String("S") ? -1 : m.captured(1).toInt();
        l.mhz = m.captured(2).toInt();
        l.active = !m.captured(3).isEmpty();
        out.append(l);
    }
    return out;
}

QList<PowerProfile> parseProfiles(const QByteArray &text)
{
    QList<PowerProfile> out;
    const QList<QByteArray> lines = text.split('\n');
    // one profile per row: "  1 3D_FULL_SCREEN *:" or " 1 3D_FULL_SCREEN*:"
    static const QRegularExpression row(QStringLiteral("^\\s*(\\d+)\\s+((?=[A-Z0-9_]*[A-Z])[A-Z0-9][A-Z0-9_]*)\\s*(\\*)?\\s*:"));
    for (const QByteArray &line : lines) {
        const auto m = row.match(QString::fromLatin1(line));
        if (m.hasMatch())
            out.append({m.captured(1).toInt(), m.captured(2), !m.captured(3).isEmpty()});
    }
    if (!out.isEmpty())
        return out;

    // one per column, in the first line: "0 BOOTUP_DEFAULT  1 3D_FULL_SCREEN* 2 ..."
    QString head;
    for (const QByteArray &line : lines) {
        if (!line.trimmed().isEmpty()) {
            head = QString::fromLatin1(line);
            break;
        }
    }
    static const QRegularExpression numbered(QStringLiteral("(\\d+)\\s+((?=[A-Z0-9_]*[A-Z])[A-Z0-9][A-Z0-9_]*)(\\s?\\*)?"));
    for (auto it = numbered.globalMatch(head); it.hasNext();) {
        const auto m = it.next();
        out.append({m.captured(1).toInt(), m.captured(2), !m.captured(3).isEmpty()});
    }
    if (!out.isEmpty())
        return out;
    static const QRegularExpression bare(QStringLiteral("((?=[A-Z0-9_]*[A-Z])[A-Z0-9][A-Z0-9_]*)(\\*)?"));
    int index = 0;
    for (auto it = bare.globalMatch(head); it.hasNext();) {
        const auto m = it.next();
        out.append({index++, m.captured(1), !m.captured(2).isEmpty()});
    }
    return out;
}

OdFanCurve parseOdFanCurve(const QByteArray &text)
{
    static const QRegularExpression point(QStringLiteral("^\\s*(\\d+):\\s*(\\d+)C\\s+(\\d+)%"));
    static const QRegularExpression tempRange(QStringLiteral("^\\s*FAN_CURVE\\([^)]*temp\\):\\s*(\\d+)C\\s+(\\d+)C"));
    static const QRegularExpression speedRange(QStringLiteral("^\\s*FAN_CURVE\\([^)]*speed\\):\\s*(\\d+)%\\s+(\\d+)%"));
    OdFanCurve c;
    bool temps = false, speeds = false;
    for (const QByteArray &raw : text.split('\n')) {
        const QString line = QString::fromLatin1(raw);
        if (auto m = point.match(line); m.hasMatch()) {
            if (m.captured(1).toInt() != c.points.size())
                return {};
            c.points.append({m.captured(2).toInt(), m.captured(3).toInt()});
        } else if (auto m = tempRange.match(line); m.hasMatch()) {
            c.tempMin = m.captured(1).toInt();
            c.tempMax = m.captured(2).toInt();
            temps = true;
        } else if (auto m = speedRange.match(line); m.hasMatch()) {
            c.speedMin = m.captured(1).toInt();
            c.speedMax = m.captured(2).toInt();
            speeds = true;
        }
    }
    c.valid = temps && speeds && c.points.size() >= 2 && c.tempMin < c.tempMax && c.speedMin < c.speedMax
              && c.speedMax <= 100;
    if (!c.valid)
        return {};
    return c;
}

namespace
{
double milli(const QString &path)
{
    const qint64 v = readNumber(path, LLONG_MIN);
    return v == LLONG_MIN ? NAN : double(v) / 1000.0;
}

int activeMhz(const QString &path)
{
    for (const DpmLevel &l : parseDpm(readFile(path)))
        if (l.active)
            return l.mhz;
    return -1;
}

// temp1..temp3 by their label: edge, junction, mem
QString tempFile(const Card &card, const QString &label)
{
    if (card.hwmon.isEmpty())
        return {};
    for (int i = 1; i <= 3; ++i) {
        const QString base = card.hwmon + QStringLiteral("/temp%1_").arg(i);
        if (readLine(base + QStringLiteral("label")) == label)
            return base;
    }
    // older kernels label nothing; their one sensor is the edge
    if (label == QLatin1String("edge") && QFile::exists(card.hwmon + QStringLiteral("/temp1_input"))
        && readLine(card.hwmon + QStringLiteral("/temp1_label")).isEmpty())
        return card.hwmon + QStringLiteral("/temp1_");
    return {};
}
} // namespace

Reading readReading(const Card &card)
{
    Reading r;
    const QString dev = card.device + QLatin1Char('/');
    const QString hw = card.hwmon + QLatin1Char('/');
    r.busy = int(readNumber(dev + QStringLiteral("gpu_busy_percent")));
    r.memBusy = int(readNumber(dev + QStringLiteral("mem_busy_percent")));
    r.vramUsed = readNumber(dev + QStringLiteral("mem_info_vram_used"));
    r.vramTotal = readNumber(dev + QStringLiteral("mem_info_vram_total"));
    r.gttUsed = readNumber(dev + QStringLiteral("mem_info_gtt_used"));
    r.gttTotal = readNumber(dev + QStringLiteral("mem_info_gtt_total"));
    if (card.hwmon.isEmpty())
        return r;

    const qint64 f1 = readNumber(hw + QStringLiteral("freq1_input"));
    r.sclk = f1 > 0 ? int(f1 / 1000000) : activeMhz(dev + QStringLiteral("pp_dpm_sclk"));
    const qint64 f2 = readNumber(hw + QStringLiteral("freq2_input"));
    r.mclk = f2 > 0 ? int(f2 / 1000000) : activeMhz(dev + QStringLiteral("pp_dpm_mclk"));

    if (const QString t = tempFile(card, QStringLiteral("edge")); !t.isEmpty())
        r.edge = milli(t + QStringLiteral("input"));
    if (const QString t = tempFile(card, QStringLiteral("junction")); !t.isEmpty())
        r.junction = milli(t + QStringLiteral("input"));
    if (const QString t = tempFile(card, QStringLiteral("mem")); !t.isEmpty())
        r.memory = milli(t + QStringLiteral("input"));

    r.fanRpm = int(readNumber(hw + QStringLiteral("fan1_input")));
    const qint64 pwm = readNumber(hw + QStringLiteral("pwm1"));
    const qint64 pwmMax = readNumber(hw + QStringLiteral("pwm1_max"), 255);
    const qint64 rpmMax = readNumber(hw + QStringLiteral("fan1_max"));
    if (pwm >= 0 && pwmMax > 0)
        r.fanPercent = int(std::lround(100.0 * double(std::min(pwm, pwmMax)) / double(pwmMax)));
    else if (r.fanRpm >= 0 && rpmMax > 0)
        r.fanPercent = int(std::lround(100.0 * double(std::min<qint64>(r.fanRpm, rpmMax)) / double(rpmMax)));

    qint64 power = readNumber(hw + QStringLiteral("power1_average"));
    if (power < 0)
        power = readNumber(hw + QStringLiteral("power1_input"));
    if (power >= 0)
        r.powerW = double(power) / 1e6;
    return r;
}

int Limits::curveTempMax() const
{
    if (odCurve.valid)
        return odCurve.tempMax;
    if (std::isnan(tempCrit))
        return 90;
    return std::clamp(int(tempCrit) - 10, 60, 100);
}

Limits readLimits(const Card &card)
{
    Limits l;
    const QString dev = card.device + QLatin1Char('/');
    const QString hw = card.hwmon + QLatin1Char('/');
    l.level = readLine(dev + QStringLiteral("power_dpm_force_performance_level"));
    l.profiles = parseProfiles(readFile(dev + QStringLiteral("pp_power_profile_mode")));
    for (const PowerProfile &p : std::as_const(l.profiles))
        if (p.name == QLatin1String("BOOTUP_DEFAULT"))
            l.defaultProfile = p.index;
    if (card.hwmon.isEmpty())
        return l;

    const qint64 cap = readNumber(hw + QStringLiteral("power1_cap"));
    const qint64 capMax = readNumber(hw + QStringLiteral("power1_cap_max"));
    if (cap >= 0 && capMax > 0) {
        l.hasPowerCap = true;
        l.capW = int(std::lround(double(cap) / 1e6));
        l.capMaxW = int(double(capMax) / 1e6);
        // many cards say 0 W here; a card starved that far crawls, desktop
        // included, so the lowest limit offered is 40 % of the top one
        l.capMinW = std::max(int(std::ceil(double(readNumber(hw + QStringLiteral("power1_cap_min"), 0)) / 1e6)),
                             int(std::lround(l.capMaxW * 0.4)));
        const qint64 def = readNumber(hw + QStringLiteral("power1_cap_default"));
        l.capDefaultW = def > 0 ? int(std::lround(double(def) / 1e6)) : -1;
        if (l.capMinW >= l.capMaxW)
            l.hasPowerCap = false;
    }

    l.hasFan = readNumber(hw + QStringLiteral("fan1_input")) >= 0;
    l.fanMinRpm = int(readNumber(hw + QStringLiteral("fan1_min")));
    l.fanMaxRpm = int(readNumber(hw + QStringLiteral("fan1_max")));
    l.odCurve = parseOdFanCurve(readFile(dev + QStringLiteral("gpu_od/fan_ctrl/fan_curve")));
    l.pwmMode = int(readNumber(hw + QStringLiteral("pwm1_enable")));
    l.pwmMin = int(readNumber(hw + QStringLiteral("pwm1_min"), 0));
    l.pwmMax = int(readNumber(hw + QStringLiteral("pwm1_max"), 255));
    // RDNA3 and later take a curve, not a fixed speed
    l.pwmControl = !l.odCurve.valid && l.hasFan && l.pwmMode >= 0 && readNumber(hw + QStringLiteral("pwm1")) >= 0
                   && l.pwmMin >= 0 && l.pwmMin < l.pwmMax && l.pwmMax <= 255;

    const QString junction = tempFile(card, QStringLiteral("junction"));
    const QString edge = tempFile(card, QStringLiteral("edge"));
    l.junctionSensor = !junction.isEmpty();
    const QString base = l.junctionSensor ? junction : edge;
    if (!base.isEmpty())
        l.tempCrit = milli(base + QStringLiteral("crit"));
    return l;
}

double curveTemperature(const Reading &r)
{
    return std::isnan(r.junction) ? r.edge : r.junction;
}

} // namespace Raye::Graphics
