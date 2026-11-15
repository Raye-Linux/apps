// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// What the Performance page reads: the processor, the memory, the disks,
// the network adapters and the graphics card, from /proc and /sys.
//
// Every function takes the text or the root it reads, so the tests hand
// it a file of their own. The rates (bytes per second, busy share) need
// two samples; the functions here only subtract them.

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace Raye::TaskManager
{

// One "cpu" or "cpuN" line of /proc/stat.
struct CpuTimes {
    qulonglong total = 0;
    qulonglong idle = 0; // idle + iowait

    bool isValid() const
    {
        return total > 0;
    }
};

// The machine's line ("cpu ") first, then one per processor ("cpuN") in
// the order the kernel prints them.
QList<CpuTimes> parseCpuTimes(const QByteArray &procStat);

// Busy share between two samples, 0 to 100.
double busyPercent(const CpuTimes &before, const CpuTimes &after);

// Fields of /proc/meminfo, in bytes.
struct MemInfo {
    qulonglong total = 0;
    qulonglong free = 0;
    qulonglong available = 0;
    qulonglong buffers = 0;
    qulonglong cached = 0;
    qulonglong reclaimable = 0; // SReclaimable
    qulonglong shmem = 0;
    qulonglong swapTotal = 0;
    qulonglong swapFree = 0;
    qulonglong committed = 0; // Committed_AS
    qulonglong commitLimit = 0;

    // what Windows calls "In use": what could not be handed to a program
    // right now without taking it from somebody
    qulonglong inUse() const
    {
        return total > available ? total - available : 0;
    }
    // the file cache, less the shared memory the kernel files under it
    qulonglong inCache() const
    {
        const qulonglong c = cached + buffers + reclaimable;
        return c > shmem ? c - shmem : 0;
    }
};

MemInfo parseMemInfo(const QByteArray &text);

// One line of /proc/diskstats: sectors are always 512 bytes there,
// whatever the disk's own sector size.
struct DiskCounters {
    QString name;
    qulonglong readBytes = 0;
    qulonglong writtenBytes = 0;
    qulonglong busyMs = 0; // field 13, time spent doing I/O
};

QList<DiskCounters> parseDiskStats(const QByteArray &text);

// The disks a person would call disks: the names under <sysRoot>/block
// that have a device behind them. Partitions are not there, and loop,
// ram, zram and the encrypted mappings have no device. A drive with no
// medium in it is not listed either.
QStringList physicalDisks(const QString &sysRoot);

struct DiskInfo {
    QString name;
    QString model;
    bool rotational = false;
    bool removable = false;
    qulonglong sizeBytes = 0;
};

DiskInfo readDiskInfo(const QString &name, const QString &sysRoot);

// One adapter of /proc/net/dev.
struct NetCounters {
    QString name;
    qulonglong receivedBytes = 0;
    qulonglong sentBytes = 0;
};

QList<NetCounters> parseNetDev(const QByteArray &text);

enum class AdapterKind { Ethernet, WiFi, Other };

// What the adapter is, from <sysRoot>/class/net/<name>: a "wireless"
// folder means Wi-Fi, a "device" link a real card. The loopback and the
// virtual ones (bridges, tunnels, containers) are Other and not shown.
AdapterKind adapterKind(const QString &name, const QString &sysRoot);

struct CpuInfo {
    QString model;
    int sockets = 0;
    int cores = 0;
    int logical = 0;
    int maxMhz = 0;
    qulonglong l1Bytes = 0;
    qulonglong l2Bytes = 0;
    qulonglong l3Bytes = 0;
    bool virtualMachine = false;
};

// The parts that do not move: the name from cpuinfo, the counts, the
// caches from <sysRoot>/devices/system/cpu. Each cache is counted once
// per group of processors that shares it, as Windows adds them up.
CpuInfo readCpuInfo(const QByteArray &cpuinfo, const QString &sysRoot);

// Average clock of every processor in MHz, now. 0 when the driver does
// not say.
int currentMhz(const QString &sysRoot);

// The same from the "cpu MHz" lines of /proc/cpuinfo, for a machine with
// no frequency driver (a virtual machine): 0 when there are none.
int cpuinfoMhz(const QByteArray &cpuinfo);

// "16K", "512K", "32768K", "8M" as sysfs writes cache sizes.
qulonglong parseCacheSize(const QString &text);

// Seconds since boot, from /proc/uptime.
qulonglong parseUptime(const QByteArray &text);

struct GpuInfo {
    QString card; // card0
    QString name;
    QString driver;
    int busyPercent = -1; // -1: the driver does not say
    qulonglong vramUsed = 0;
    qulonglong vramTotal = 0;
};

// The graphics cards under <sysRoot>/class/drm, one per cardN that has a
// PCI device. pciIds is the text of pci.ids, for the names; empty is
// fine, the name is then the driver's.
QList<GpuInfo> readGpus(const QString &sysRoot, const QByteArray &pciIds);

// "Vendor Device" for two hex ids, from pci.ids. Empty when unknown.
QString pciName(const QByteArray &pciIds, const QString &vendor, const QString &device);

} // namespace Raye::TaskManager
