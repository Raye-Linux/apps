// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The readings of the Performance page, against files of their own.

#include "sysinfo.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace Raye::TaskManager;

namespace
{
void write(const QString &path, const QByteArray &text)
{
    QDir().mkpath(path.left(path.lastIndexOf(QLatin1Char('/'))));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(text);
}
}

class TestSysInfo : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cpuLinesInOrder()
    {
        const QByteArray stat = "cpu  100 0 50 800 50 0 0 0 0 0\n"
                                "cpu0 50 0 25 400 25 0 0 0 0 0\n"
                                "cpu1 50 0 25 400 25 0 0 0 0 0\n"
                                "intr 1 2 3\n";
        const QList<CpuTimes> t = parseCpuTimes(stat);
        QCOMPARE(t.size(), 3);
        QCOMPARE(t.at(0).total, 1000ull);
        QCOMPARE(t.at(0).idle, 850ull);
        QCOMPARE(t.at(1).total, 500ull);
    }

    void busyShare()
    {
        const CpuTimes a{1000, 800};
        const CpuTimes b{2000, 1300};
        QCOMPARE(busyPercent(a, b), 50.0);
        // no time passed, or the counters went back: nothing, not a crash
        QCOMPARE(busyPercent(b, b), 0.0);
        QCOMPARE(busyPercent(b, a), 0.0);
    }

    void memInfo()
    {
        const QByteArray text = "MemTotal:        8000000 kB\n"
                                "MemFree:         1000000 kB\n"
                                "MemAvailable:    5000000 kB\n"
                                "Buffers:          100000 kB\n"
                                "Cached:          2000000 kB\n"
                                "SReclaimable:     200000 kB\n"
                                "Shmem:            300000 kB\n"
                                "SwapTotal:       2000000 kB\n"
                                "SwapFree:        1500000 kB\n"
                                "CommitLimit:     6000000 kB\n"
                                "Committed_AS:    4000000 kB\n";
        const MemInfo m = parseMemInfo(text);
        QCOMPARE(m.total, 8000000ull * 1024);
        QCOMPARE(m.inUse(), 3000000ull * 1024);
        QCOMPARE(m.inCache(), 2000000ull * 1024);
        QCOMPARE(m.committed, 4000000ull * 1024);
        QCOMPARE(m.commitLimit, 6000000ull * 1024);
        QCOMPARE(m.swapFree, 1500000ull * 1024);
    }

    void diskStats()
    {
        const QByteArray text = " 259 0 nvme0n1 100 0 2048 10 50 0 4096 20 0 1234 30 0 0 0 0\n"
                                "   7 0 loop0 1 0 8 0 0 0 0 0 0 0 0\n";
        const QList<DiskCounters> d = parseDiskStats(text);
        QCOMPARE(d.size(), 2);
        QCOMPARE(d.at(0).name, QStringLiteral("nvme0n1"));
        QCOMPARE(d.at(0).readBytes, 2048ull * 512);
        QCOMPARE(d.at(0).writtenBytes, 4096ull * 512);
        QCOMPARE(d.at(0).busyMs, 1234ull);
    }

    void onlyRealDisksAreDisks()
    {
        QTemporaryDir root;
        const QString sys = root.path();
        write(sys + QStringLiteral("/block/nvme0n1/device/model"), "Raye SSD 512\n");
        write(sys + QStringLiteral("/block/nvme0n1/size"), "1000\n");
        write(sys + QStringLiteral("/block/nvme0n1/queue/rotational"), "0\n");
        write(sys + QStringLiteral("/block/sda/device/model"), "Big disk\n");
        write(sys + QStringLiteral("/block/sda/queue/rotational"), "1\n");
        write(sys + QStringLiteral("/block/loop0/size"), "8\n");
        write(sys + QStringLiteral("/block/dm-0/size"), "8\n");
        write(sys + QStringLiteral("/block/sda/size"), "2000\n");
        // an empty CD tray: a device, and nothing to read
        write(sys + QStringLiteral("/block/sr0/device/model"), "DVD\n");
        write(sys + QStringLiteral("/block/sr0/size"), "0\n");
        QCOMPARE(physicalDisks(sys), (QStringList{QStringLiteral("nvme0n1"), QStringLiteral("sda")}));

        const DiskInfo info = readDiskInfo(QStringLiteral("nvme0n1"), sys);
        QCOMPARE(info.model, QStringLiteral("Raye SSD 512"));
        QCOMPARE(info.sizeBytes, 512000ull);
        QVERIFY(!info.rotational);
        QVERIFY(readDiskInfo(QStringLiteral("sda"), sys).rotational);
    }

    void netDev()
    {
        const QByteArray text = "Inter-|   Receive                                                |  Transmit\n"
                                " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets\n"
                                "    lo:   1000      10    0    0    0     0          0         0     1000      10    0    0    0     0       0          0\n"
                                "  eth0: 123456     100    0    0    0     0          0         0    65432      50    0    0    0     0       0          0\n";
        const QList<NetCounters> n = parseNetDev(text);
        QCOMPARE(n.size(), 2);
        QCOMPARE(n.at(1).name, QStringLiteral("eth0"));
        QCOMPARE(n.at(1).receivedBytes, 123456ull);
        QCOMPARE(n.at(1).sentBytes, 65432ull);
    }

    void adapterKinds()
    {
        QTemporaryDir root;
        const QString sys = root.path();
        write(sys + QStringLiteral("/class/net/wlan0/device/vendor"), "0x8086\n");
        write(sys + QStringLiteral("/class/net/wlan0/wireless/x"), "");
        write(sys + QStringLiteral("/class/net/enp1s0/device/vendor"), "0x1af4\n");
        write(sys + QStringLiteral("/class/net/virbr0/bridge/x"), "");
        QCOMPARE(adapterKind(QStringLiteral("wlan0"), sys), AdapterKind::WiFi);
        QCOMPARE(adapterKind(QStringLiteral("enp1s0"), sys), AdapterKind::Ethernet);
        QCOMPARE(adapterKind(QStringLiteral("virbr0"), sys), AdapterKind::Other);
        QCOMPARE(adapterKind(QStringLiteral("lo"), sys), AdapterKind::Other);
    }

    void cacheSizes()
    {
        QCOMPARE(parseCacheSize(QStringLiteral("32K")), 32768ull);
        QCOMPARE(parseCacheSize(QStringLiteral("8M\n")), 8ull * 1024 * 1024);
        QCOMPARE(parseCacheSize(QStringLiteral("512")), 512ull);
        QCOMPARE(parseCacheSize(QStringLiteral("x")), 0ull);
    }

    void cpuCountsAndSharedCaches()
    {
        QTemporaryDir root;
        const QString sys = root.path();
        // two cores, two threads each; L1d and L2 per core, one L3 for all
        for (int cpu = 0; cpu < 4; ++cpu) {
            const QString base = sys + QStringLiteral("/devices/system/cpu/cpu%1/cache/").arg(cpu);
            const QByteArray core = cpu < 2 ? "0-1\n" : "2-3\n";
            write(base + QStringLiteral("index0/level"), "1\n");
            write(base + QStringLiteral("index0/type"), "Data\n");
            write(base + QStringLiteral("index0/size"), "32K\n");
            write(base + QStringLiteral("index0/shared_cpu_list"), core);
            write(base + QStringLiteral("index2/level"), "2\n");
            write(base + QStringLiteral("index2/type"), "Unified\n");
            write(base + QStringLiteral("index2/size"), "512K\n");
            write(base + QStringLiteral("index2/shared_cpu_list"), core);
            write(base + QStringLiteral("index3/level"), "3\n");
            write(base + QStringLiteral("index3/type"), "Unified\n");
            write(base + QStringLiteral("index3/size"), "8192K\n");
            write(base + QStringLiteral("index3/shared_cpu_list"), "0-3\n");
            write(sys + QStringLiteral("/devices/system/cpu/cpu%1/cpufreq/cpuinfo_max_freq").arg(cpu), "3600000\n");
            write(sys + QStringLiteral("/devices/system/cpu/cpu%1/cpufreq/scaling_cur_freq").arg(cpu), cpu < 2 ? "2000000\n" : "3000000\n");
        }
        QByteArray cpuinfo;
        for (int cpu = 0; cpu < 4; ++cpu) {
            cpuinfo += "processor\t: " + QByteArray::number(cpu) + "\nmodel name\t: Raye Test CPU\nphysical id\t: 0\n"
                       "core id\t\t: " + QByteArray::number(cpu / 2) + "\nflags\t\t: fpu hypervisor\n\n";
        }
        const CpuInfo info = readCpuInfo(cpuinfo, sys);
        QCOMPARE(info.model, QStringLiteral("Raye Test CPU"));
        QCOMPARE(info.sockets, 1);
        QCOMPARE(info.cores, 2);
        QCOMPARE(info.logical, 4);
        QCOMPARE(info.maxMhz, 3600);
        QCOMPARE(info.l1Bytes, 2ull * 32 * 1024);
        QCOMPARE(info.l2Bytes, 2ull * 512 * 1024);
        QCOMPARE(info.l3Bytes, 8192ull * 1024);
        QVERIFY(info.virtualMachine);
        QCOMPARE(currentMhz(sys), 2500);
    }

    void mhzFromCpuinfo()
    {
        QCOMPARE(cpuinfoMhz("processor\t: 0\ncpu MHz\t\t: 1996.000\n\nprocessor\t: 1\ncpu MHz\t\t: 2004.000\n"), 2000);
        QCOMPARE(cpuinfoMhz(""), 0);
    }

    void uptime()
    {
        QCOMPARE(parseUptime("12345.67 9999.00\n"), 12345ull);
        QCOMPARE(parseUptime(""), 0ull);
    }

    void pciNames()
    {
        const QByteArray ids = "# comment\n"
                               "1002  Advanced Micro Devices, Inc. [AMD/ATI]\n"
                               "\t73bf  Navi 21 [Radeon RX 6800/6800 XT / 6900 XT]\n"
                               "\t\t1002 0e3a  subsystem\n"
                               "8086  Intel Corporation\n"
                               "\t9a49  TigerLake-LP GT2 [Iris Xe Graphics]\n";
        QCOMPARE(pciName(ids, QStringLiteral("0x8086"), QStringLiteral("0x9a49")),
                 QStringLiteral("Intel Corporation TigerLake-LP GT2 [Iris Xe Graphics]"));
        QCOMPARE(pciName(ids, QStringLiteral("0x8086"), QStringLiteral("0x0000")), QString());
    }

    void gpus()
    {
        QTemporaryDir root;
        const QString sys = root.path();
        write(sys + QStringLiteral("/class/drm/card0/device/vendor"), "0x1002\n");
        write(sys + QStringLiteral("/class/drm/card0/device/device"), "0x73bf\n");
        write(sys + QStringLiteral("/class/drm/card0/device/gpu_busy_percent"), "37\n");
        write(sys + QStringLiteral("/class/drm/card0/device/mem_info_vram_used"), "1048576\n");
        write(sys + QStringLiteral("/class/drm/card0/device/mem_info_vram_total"), "16777216\n");
        write(sys + QStringLiteral("/class/drm/card0/device/uevent"), "DRIVER=amdgpu\nPCI_ID=1002:73BF\n");
        // a connector, not a card
        write(sys + QStringLiteral("/class/drm/card0-DP-1/status"), "connected\n");
        // a card with no busy figure
        write(sys + QStringLiteral("/class/drm/card1/device/vendor"), "0x8086\n");
        write(sys + QStringLiteral("/class/drm/card1/device/device"), "0x9a49\n");
        write(sys + QStringLiteral("/class/drm/card1/device/uevent"), "DRIVER=i915\n");
        const QList<GpuInfo> g = readGpus(sys, QByteArray());
        QCOMPARE(g.size(), 2);
        QCOMPARE(g.at(0).card, QStringLiteral("card0"));
        QCOMPARE(g.at(0).driver, QStringLiteral("amdgpu"));
        QCOMPARE(g.at(0).busyPercent, 37);
        QCOMPARE(g.at(0).vramTotal, 16777216ull);
        QCOMPARE(g.at(1).busyPercent, -1);
        QCOMPARE(g.at(1).name, QStringLiteral("i915"));
    }
};

QTEST_GUILESS_MAIN(TestSysInfo)
#include "test_sysinfo.moc"
