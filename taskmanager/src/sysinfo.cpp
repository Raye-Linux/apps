// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sysinfo.h"

#include <QDir>
#include <QFile>
#include <QSet>

namespace Raye::TaskManager
{

namespace
{
QByteArray readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

QString readLine(const QString &path)
{
    return QString::fromUtf8(readFile(path)).trimmed();
}

qulonglong readNumber(const QString &path)
{
    return readLine(path).toULongLong();
}
}

QList<CpuTimes> parseCpuTimes(const QByteArray &procStat)
{
    QList<CpuTimes> out;
    for (const QByteArray &line : procStat.split('\n')) {
        if (!line.startsWith("cpu")) {
            continue;
        }
        const QList<QByteArray> parts = line.simplified().split(' ');
        if (parts.size() < 5) {
            continue;
        }
        CpuTimes t;
        // user nice system idle iowait irq softirq steal; guest and
        // guest_nice are already counted inside user and nice
        for (int i = 1; i < parts.size() && i <= 8; ++i) {
            const qulonglong v = parts.at(i).toULongLong();
            t.total += v;
            if (i == 4 || i == 5) {
                t.idle += v;
            }
        }
        out.append(t);
    }
    return out;
}

double busyPercent(const CpuTimes &before, const CpuTimes &after)
{
    if (after.total <= before.total || after.idle < before.idle) {
        return 0.0;
    }
    const double total = double(after.total - before.total);
    const double idle = double(after.idle - before.idle);
    return qBound(0.0, (total - idle) / total * 100.0, 100.0);
}

MemInfo parseMemInfo(const QByteArray &text)
{
    MemInfo m;
    for (const QByteArray &line : text.split('\n')) {
        const int colon = line.indexOf(':');
        if (colon < 0) {
            continue;
        }
        const QByteArray key = line.left(colon);
        const qulonglong kib = line.mid(colon + 1).simplified().split(' ').value(0).toULongLong();
        const qulonglong bytes = kib * 1024;
        if (key == "MemTotal") {
            m.total = bytes;
        } else if (key == "MemFree") {
            m.free = bytes;
        } else if (key == "MemAvailable") {
            m.available = bytes;
        } else if (key == "Buffers") {
            m.buffers = bytes;
        } else if (key == "Cached") {
            m.cached = bytes;
        } else if (key == "SReclaimable") {
            m.reclaimable = bytes;
        } else if (key == "Shmem") {
            m.shmem = bytes;
        } else if (key == "SwapTotal") {
            m.swapTotal = bytes;
        } else if (key == "SwapFree") {
            m.swapFree = bytes;
        } else if (key == "Committed_AS") {
            m.committed = bytes;
        } else if (key == "CommitLimit") {
            m.commitLimit = bytes;
        }
    }
    return m;
}

QList<DiskCounters> parseDiskStats(const QByteArray &text)
{
    QList<DiskCounters> out;
    for (const QByteArray &line : text.split('\n')) {
        const QList<QByteArray> p = line.simplified().split(' ');
        if (p.size() < 14) {
            continue;
        }
        DiskCounters d;
        d.name = QString::fromUtf8(p.at(2));
        d.readBytes = p.at(5).toULongLong() * 512;
        d.writtenBytes = p.at(9).toULongLong() * 512;
        d.busyMs = p.at(12).toULongLong();
        out.append(d);
    }
    return out;
}

QStringList physicalDisks(const QString &sysRoot)
{
    QStringList out;
    const QDir block(sysRoot + QStringLiteral("/block"));
    const QStringList names = block.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System, QDir::Name);
    for (const QString &name : names) {
        // a drive with nothing in it (an empty CD tray) has a size of 0
        if (QFileInfo::exists(block.filePath(name + QStringLiteral("/device")))
            && readNumber(block.filePath(name + QStringLiteral("/size"))) > 0) {
            out.append(name);
        }
    }
    return out;
}

DiskInfo readDiskInfo(const QString &name, const QString &sysRoot)
{
    const QString base = sysRoot + QStringLiteral("/block/") + name;
    DiskInfo d;
    d.name = name;
    d.model = readLine(base + QStringLiteral("/device/model"));
    d.rotational = readLine(base + QStringLiteral("/queue/rotational")) == QLatin1String("1");
    d.removable = readLine(base + QStringLiteral("/removable")) == QLatin1String("1");
    // "size" is in 512-byte sectors, whatever the disk's own sector size
    d.sizeBytes = readNumber(base + QStringLiteral("/size")) * 512;
    return d;
}

QList<NetCounters> parseNetDev(const QByteArray &text)
{
    QList<NetCounters> out;
    for (const QByteArray &line : text.split('\n')) {
        const int colon = line.indexOf(':');
        if (colon < 0 || line.contains('|')) {
            continue;
        }
        const QList<QByteArray> p = line.mid(colon + 1).simplified().split(' ');
        if (p.size() < 9) {
            continue;
        }
        NetCounters n;
        n.name = QString::fromUtf8(line.left(colon).trimmed());
        n.receivedBytes = p.at(0).toULongLong();
        n.sentBytes = p.at(8).toULongLong();
        out.append(n);
    }
    return out;
}

AdapterKind adapterKind(const QString &name, const QString &sysRoot)
{
    const QString base = sysRoot + QStringLiteral("/class/net/") + name;
    if (QFileInfo::exists(base + QStringLiteral("/wireless")) || QFileInfo::exists(base + QStringLiteral("/phy80211"))) {
        return AdapterKind::WiFi;
    }
    if (QFileInfo::exists(base + QStringLiteral("/device"))) {
        return AdapterKind::Ethernet;
    }
    return AdapterKind::Other;
}

qulonglong parseCacheSize(const QString &text)
{
    QString t = text.trimmed();
    qulonglong unit = 1;
    if (t.endsWith(QLatin1Char('K'))) {
        unit = 1024;
        t.chop(1);
    } else if (t.endsWith(QLatin1Char('M'))) {
        unit = 1024 * 1024;
        t.chop(1);
    }
    bool ok = false;
    const qulonglong v = t.toULongLong(&ok);
    return ok ? v * unit : 0;
}

CpuInfo readCpuInfo(const QByteArray &cpuinfo, const QString &sysRoot)
{
    CpuInfo info;
    QSet<QString> sockets;
    QSet<QString> cores;
    QString socket;
    for (const QByteArray &raw : cpuinfo.split('\n')) {
        const int colon = raw.indexOf(':');
        if (colon < 0) {
            continue;
        }
        const QByteArray key = raw.left(colon).trimmed();
        const QString value = QString::fromUtf8(raw.mid(colon + 1)).trimmed();
        if (key == "processor") {
            ++info.logical;
            socket = QStringLiteral("0");
        } else if (key == "model name" && info.model.isEmpty()) {
            info.model = value;
        } else if (key == "physical id") {
            socket = value;
            sockets.insert(value);
        } else if (key == "core id") {
            cores.insert(socket + QLatin1Char('/') + value);
        } else if (key == "flags" && value.split(QLatin1Char(' ')).contains(QLatin1String("hypervisor"))) {
            info.virtualMachine = true;
        }
    }
    info.sockets = qMax(1, int(sockets.size()));
    info.cores = cores.isEmpty() ? info.logical : int(cores.size());

    const QString cpuDir = sysRoot + QStringLiteral("/devices/system/cpu");
    // each cache once per group of processors sharing it
    QSet<QString> seen;
    for (int cpu = 0; cpu < info.logical; ++cpu) {
        const QDir cache(cpuDir + QStringLiteral("/cpu%1/cache").arg(cpu));
        const QStringList indexes = cache.entryList({QStringLiteral("index*")}, QDir::Dirs);
        for (const QString &index : indexes) {
            const QString base = cache.filePath(index) + QLatin1Char('/');
            const QString level = readLine(base + QStringLiteral("level"));
            const QString type = readLine(base + QStringLiteral("type"));
            const QString shared = readLine(base + QStringLiteral("shared_cpu_list"));
            // L1 counts data and instruction together, as Windows does
            const QString key = level + QLatin1Char('/') + type + QLatin1Char('/') + shared;
            if (seen.contains(key)) {
                continue;
            }
            seen.insert(key);
            const qulonglong size = parseCacheSize(readLine(base + QStringLiteral("size")));
            if (level == QLatin1String("1")) {
                info.l1Bytes += size;
            } else if (level == QLatin1String("2")) {
                info.l2Bytes += size;
            } else if (level == QLatin1String("3")) {
                info.l3Bytes += size;
            }
        }
    }
    info.maxMhz = int(readNumber(cpuDir + QStringLiteral("/cpu0/cpufreq/cpuinfo_max_freq")) / 1000);
    return info;
}

int currentMhz(const QString &sysRoot)
{
    const QDir cpus(sysRoot + QStringLiteral("/devices/system/cpu"));
    qulonglong sum = 0;
    int count = 0;
    const QStringList names = cpus.entryList({QStringLiteral("cpu*")}, QDir::Dirs);
    for (const QString &name : names) {
        bool numeric = false;
        name.mid(3).toInt(&numeric);
        if (!numeric) {
            continue;
        }
        const qulonglong khz = readNumber(cpus.filePath(name + QStringLiteral("/cpufreq/scaling_cur_freq")));
        if (khz > 0) {
            sum += khz;
            ++count;
        }
    }
    return count > 0 ? int(sum / count / 1000) : 0;
}

int cpuinfoMhz(const QByteArray &cpuinfo)
{
    double sum = 0;
    int count = 0;
    for (const QByteArray &line : cpuinfo.split('\n')) {
        if (line.startsWith("cpu MHz")) {
            sum += line.mid(line.indexOf(':') + 1).trimmed().toDouble();
            ++count;
        }
    }
    return count > 0 ? int(sum / count) : 0;
}

qulonglong parseUptime(const QByteArray &text)
{
    return qulonglong(text.split(' ').value(0).toDouble());
}

QString pciName(const QByteArray &pciIds, const QString &vendor, const QString &device)
{
    const QByteArray v = vendor.toLatin1().toLower().replace("0x", "");
    const QByteArray d = device.toLatin1().toLower().replace("0x", "");
    if (v.isEmpty() || d.isEmpty()) {
        return {};
    }
    QString vendorName;
    bool inVendor = false;
    for (const QByteArray &line : pciIds.split('\n')) {
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        if (line.at(0) != '\t') {
            if (inVendor) {
                break;
            }
            if (line.startsWith(QByteArray(v + "  "))) {
                inVendor = true;
                vendorName = QString::fromUtf8(line.mid(v.size() + 2)).trimmed();
            }
            continue;
        }
        if (inVendor && !line.startsWith("\t\t") && line.mid(1).startsWith(QByteArray(d + "  "))) {
            return vendorName + QLatin1Char(' ') + QString::fromUtf8(line.mid(1 + d.size() + 2)).trimmed();
        }
    }
    return {};
}

QList<GpuInfo> readGpus(const QString &sysRoot, const QByteArray &pciIds)
{
    QList<GpuInfo> out;
    const QDir drm(sysRoot + QStringLiteral("/class/drm"));
    const QStringList names = drm.entryList({QStringLiteral("card*")}, QDir::Dirs | QDir::System, QDir::Name);
    for (const QString &card : names) {
        bool numeric = false;
        card.mid(4).toInt(&numeric);
        // card0-DP-1 and the like are connectors of a card, not cards
        if (!numeric) {
            continue;
        }
        const QString dev = drm.filePath(card + QStringLiteral("/device/"));
        const QString vendor = readLine(dev + QStringLiteral("vendor"));
        if (vendor.isEmpty()) {
            continue;
        }
        GpuInfo g;
        g.card = card;
        const QByteArray uevent = readFile(dev + QStringLiteral("uevent"));
        for (const QByteArray &line : uevent.split('\n')) {
            if (line.startsWith("DRIVER=")) {
                g.driver = QString::fromUtf8(line.mid(7)).trimmed();
            }
        }
        g.name = pciName(pciIds, vendor, readLine(dev + QStringLiteral("device")));
        if (g.name.isEmpty()) {
            g.name = g.driver;
        }
        const QString busy = readLine(dev + QStringLiteral("gpu_busy_percent"));
        bool ok = false;
        const int value = busy.toInt(&ok);
        g.busyPercent = ok ? value : -1;
        g.vramUsed = readNumber(dev + QStringLiteral("mem_info_vram_used"));
        g.vramTotal = readNumber(dev + QStringLiteral("mem_info_vram_total"));
        out.append(g);
    }
    return out;
}

} // namespace Raye::TaskManager
