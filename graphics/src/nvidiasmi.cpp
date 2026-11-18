// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nvidiasmi.h"

namespace Raye::Graphics
{

namespace
{
// the order of the columns parseNvidiaSmi reads
const char *const fields[] = {"name", "driver_version", "pci.bus_id", "utilization.gpu", "utilization.memory",
                              "clocks.gr", "clocks.mem", "memory.used", "memory.total", "temperature.gpu",
                              "fan.speed", "power.draw", "power.limit"};
constexpr int fieldCount = int(sizeof(fields) / sizeof(fields[0]));

// "[N/A]", "[Not Supported]" and the like are no value
bool missing(const QString &v)
{
    return v.isEmpty() || v.startsWith(QLatin1Char('['));
}
int whole(const QString &v)
{
    bool ok = false;
    const double d = v.toDouble(&ok);
    return ok && !missing(v) ? int(std::lround(d)) : -1;
}
double real(const QString &v)
{
    bool ok = false;
    const double d = v.toDouble(&ok);
    return ok && !missing(v) ? d : NAN;
}
qint64 mib(const QString &v)
{
    const int n = whole(v);
    return n < 0 ? -1 : qint64(n) * 1024 * 1024;
}
} // namespace

QStringList nvidiaSmiArguments()
{
    QStringList names;
    for (const char *f : fields)
        names.append(QLatin1String(f));
    return {QStringLiteral("--query-gpu=") + names.join(QLatin1Char(',')),
            QStringLiteral("--format=csv,noheader,nounits")};
}

QList<NvidiaReading> parseNvidiaSmi(const QByteArray &csv)
{
    QList<NvidiaReading> out;
    for (const QByteArray &raw : csv.split('\n')) {
        const QString line = QString::fromUtf8(raw).trimmed();
        if (line.isEmpty())
            continue;
        QStringList v = line.split(QLatin1Char(','));
        if (v.size() != fieldCount)
            continue;
        for (QString &s : v)
            s = s.trimmed();
        NvidiaReading r;
        r.name = v.at(0);
        r.driver = missing(v.at(1)) ? QString() : v.at(1);
        // 00000000:01:00.0 -> 0000:01:00.0
        QString bus = v.at(2).toLower();
        if (bus.size() == 16 && bus.startsWith(QLatin1String("0000")))
            bus = bus.mid(4);
        r.slot = bus;
        r.busy = whole(v.at(3));
        r.memBusy = whole(v.at(4));
        r.sclk = whole(v.at(5));
        r.mclk = whole(v.at(6));
        r.vramUsed = mib(v.at(7));
        r.vramTotal = mib(v.at(8));
        r.temp = real(v.at(9));
        r.fanPercent = whole(v.at(10));
        r.powerW = real(v.at(11));
        r.powerLimitW = real(v.at(12));
        out.append(r);
    }
    return out;
}

} // namespace Raye::Graphics
