// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "performancemodel.h"

#include <QDateTime>
#include <QFile>
#include <QNetworkInterface>

namespace Raye::TaskManager
{

namespace
{
constexpr int HistoryLength = 60;
constexpr int IntervalMs = 1000;
// minimised: the graphs are not looked at
constexpr int IdleIntervalMs = 5000;

QByteArray readWhole(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

QString addressOf(const QString &adapter, QAbstractSocket::NetworkLayerProtocol protocol)
{
    const QNetworkInterface iface = QNetworkInterface::interfaceFromName(adapter);
    for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
        if (entry.ip().protocol() == protocol && !entry.ip().isLinkLocal()) {
            return entry.ip().toString();
        }
    }
    return {};
}
}

void PerformanceModel::Series::push(double v, int length)
{
    values.append(v);
    while (values.size() > length) {
        values.removeFirst();
    }
}

QVariantList PerformanceModel::Series::toList() const
{
    QVariantList out;
    out.reserve(HistoryLength);
    // the graph always has a full minute: what was not measured yet is 0
    for (int i = int(values.size()); i < HistoryLength; ++i) {
        out.append(0.0);
    }
    for (double v : values) {
        out.append(v);
    }
    return out;
}

PerformanceModel::PerformanceModel(QObject *parent)
    : QObject(parent)
{
    m_cpuInfo = readCpuInfo(readWhole(QStringLiteral("/proc/cpuinfo")), QStringLiteral("/sys"));
    m_timer.setInterval(IntervalMs);
    connect(&m_timer, &QTimer::timeout, this, &PerformanceModel::refresh);
    m_timer.start();
    refresh();
}

bool PerformanceModel::active() const
{
    return m_active;
}

void PerformanceModel::setActive(bool active)
{
    if (active == m_active) {
        return;
    }
    m_active = active;
    m_timer.setInterval(active ? IntervalMs : IdleIntervalMs);
    Q_EMIT activeChanged();
}

int PerformanceModel::historyLength() const
{
    return HistoryLength;
}

void PerformanceModel::refresh()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const double seconds = m_lastAt > 0 ? double(now - m_lastAt) / 1000.0 : 0.0;
    m_lastAt = now;

    const QList<CpuTimes> cpu = parseCpuTimes(readWhole(QStringLiteral("/proc/stat")));
    if (!cpu.isEmpty()) {
        m_cpu = m_cpuBefore.isValid() ? busyPercent(m_cpuBefore, cpu.first()) : 0.0;
        m_cpuBefore = cpu.first();
    }
    m_cpuSeries.push(m_cpu, HistoryLength);
    m_mhz = currentMhz(QStringLiteral("/sys"));
    if (m_mhz == 0) {
        m_mhz = cpuinfoMhz(readWhole(QStringLiteral("/proc/cpuinfo")));
    }
    m_uptime = parseUptime(readWhole(QStringLiteral("/proc/uptime")));
    // the fourth field of loadavg is "running/total" scheduling entities,
    // which are the threads
    m_threads = QString::fromLatin1(readWhole(QStringLiteral("/proc/loadavg"))).section(QLatin1Char(' '), 3, 3).section(QLatin1Char('/'), 1).toInt();

    m_mem = parseMemInfo(readWhole(QStringLiteral("/proc/meminfo")));
    m_memSeries.push(m_mem.total > 0 ? double(m_mem.inUse()) * 100.0 / double(m_mem.total) : 0.0, HistoryLength);

    // disks
    const QStringList physical = physicalDisks(QStringLiteral("/sys"));
    QHash<QString, DiskCounters> diskNow;
    for (const DiskCounters &d : parseDiskStats(readWhole(QStringLiteral("/proc/diskstats")))) {
        if (physical.contains(d.name)) {
            diskNow.insert(d.name, d);
        }
    }
    m_disks.clear();
    m_diskPercent = 0.0;
    int number = 0;
    for (const QString &name : physical) {
        if (!m_diskInfo.contains(name)) {
            m_diskInfo.insert(name, readDiskInfo(name, QStringLiteral("/sys")));
        }
        const DiskInfo &info = m_diskInfo[name];
        const DiskCounters d = diskNow.value(name);
        const DiskCounters b = m_diskBefore.value(name);
        double active = 0, readRate = 0, writeRate = 0;
        if (seconds > 0 && m_diskBefore.contains(name)) {
            active = qBound(0.0, double(d.busyMs - b.busyMs) / (seconds * 10.0), 100.0);
            readRate = double(d.readBytes - b.readBytes) / seconds;
            writeRate = double(d.writtenBytes - b.writtenBytes) / seconds;
        }
        m_diskPercent = qMax(m_diskPercent, active);
        Series &series = m_series[QStringLiteral("disk:") + name];
        series.push(active, HistoryLength);
        m_disks.append(QVariantMap{
            {QStringLiteral("key"), QString(QStringLiteral("disk:") + name)},
            {QStringLiteral("number"), number++},
            {QStringLiteral("name"), name},
            {QStringLiteral("model"), info.model},
            {QStringLiteral("solidState"), !info.rotational},
            {QStringLiteral("removable"), info.removable},
            {QStringLiteral("capacity"), double(info.sizeBytes)},
            {QStringLiteral("active"), active},
            {QStringLiteral("read"), readRate},
            {QStringLiteral("write"), writeRate},
            {QStringLiteral("history"), series.toList()},
        });
    }
    m_diskBefore = diskNow;

    // network adapters; their addresses cost a netlink round trip, so
    // they are read every tenth time
    if (m_addressAge-- <= 0) {
        m_addresses.clear();
        m_addressAge = 10;
    }
    QHash<QString, NetCounters> netNow;
    m_adapters.clear();
    m_netBits = 0.0;
    for (const NetCounters &n : parseNetDev(readWhole(QStringLiteral("/proc/net/dev")))) {
        const AdapterKind kind = adapterKind(n.name, QStringLiteral("/sys"));
        if (kind == AdapterKind::Other) {
            continue;
        }
        netNow.insert(n.name, n);
        const NetCounters b = m_netBefore.value(n.name);
        double sent = 0, received = 0;
        if (seconds > 0 && m_netBefore.contains(n.name) && n.sentBytes >= b.sentBytes && n.receivedBytes >= b.receivedBytes) {
            sent = double(n.sentBytes - b.sentBytes) * 8.0 / seconds;
            received = double(n.receivedBytes - b.receivedBytes) * 8.0 / seconds;
        }
        m_netBits += sent + received;
        Series &series = m_series[QStringLiteral("net:") + n.name];
        series.push(sent + received, HistoryLength);
        if (!m_addresses.contains(n.name)) {
            m_addresses.insert(n.name, {addressOf(n.name, QAbstractSocket::IPv4Protocol), addressOf(n.name, QAbstractSocket::IPv6Protocol)});
        }
        const QStringList addresses = m_addresses.value(n.name);
        const QString operstate = QString::fromLatin1(readWhole(QStringLiteral("/sys/class/net/%1/operstate").arg(n.name))).trimmed();
        const int speed = QString::fromLatin1(readWhole(QStringLiteral("/sys/class/net/%1/speed").arg(n.name))).trimmed().toInt();
        m_adapters.append(QVariantMap{
            {QStringLiteral("key"), QString(QStringLiteral("net:") + n.name)},
            {QStringLiteral("name"), n.name},
            {QStringLiteral("wifi"), kind == AdapterKind::WiFi},
            {QStringLiteral("connected"), operstate == QLatin1String("up")},
            {QStringLiteral("linkMbps"), speed > 0 ? speed : 0},
            {QStringLiteral("sent"), sent},
            {QStringLiteral("received"), received},
            {QStringLiteral("ipv4"), addresses.value(0)},
            {QStringLiteral("ipv6"), addresses.value(1)},
            {QStringLiteral("history"), series.toList()},
        });
    }
    m_netBefore = netNow;

    // graphics cards; pci.ids is read once, and only when there is a card
    m_gpus.clear();
    QList<GpuInfo> cards = readGpus(QStringLiteral("/sys"), m_pciIds);
    if (!cards.isEmpty() && !m_pciIdsRead) {
        m_pciIdsRead = true;
        m_pciIds = readWhole(QStringLiteral("/usr/share/misc/pci.ids"));
        cards = readGpus(QStringLiteral("/sys"), m_pciIds);
    }
    for (const GpuInfo &g : std::as_const(cards)) {
        Series &series = m_series[QStringLiteral("gpu:") + g.card];
        series.push(qMax(0, g.busyPercent), HistoryLength);
        m_gpus.append(QVariantMap{
            {QStringLiteral("key"), QString(QStringLiteral("gpu:") + g.card)},
            {QStringLiteral("name"), g.name},
            {QStringLiteral("driver"), g.driver},
            {QStringLiteral("busy"), g.busyPercent},
            {QStringLiteral("vramUsed"), double(g.vramUsed)},
            {QStringLiteral("vramTotal"), double(g.vramTotal)},
            {QStringLiteral("history"), series.toList()},
        });
    }

    m_diskList.setDevices(m_disks);
    m_adapterList.setDevices(m_adapters);
    m_gpuList.setDevices(m_gpus);
    Q_EMIT sampled();
}

double PerformanceModel::cpuPercent() const
{
    return m_cpu;
}

QVariantList PerformanceModel::cpuHistory() const
{
    return m_cpuSeries.toList();
}

int PerformanceModel::cpuMhz() const
{
    return m_mhz;
}

QVariantMap PerformanceModel::cpuInfo() const
{
    return {
        {QStringLiteral("model"), m_cpuInfo.model},
        {QStringLiteral("sockets"), m_cpuInfo.sockets},
        {QStringLiteral("cores"), m_cpuInfo.cores},
        {QStringLiteral("logical"), m_cpuInfo.logical},
        {QStringLiteral("maxMhz"), m_cpuInfo.maxMhz},
        {QStringLiteral("l1"), double(m_cpuInfo.l1Bytes)},
        {QStringLiteral("l2"), double(m_cpuInfo.l2Bytes)},
        {QStringLiteral("l3"), double(m_cpuInfo.l3Bytes)},
        {QStringLiteral("virtualMachine"), m_cpuInfo.virtualMachine},
    };
}

qulonglong PerformanceModel::uptimeSeconds() const
{
    return m_uptime;
}

int PerformanceModel::threadCount() const
{
    return m_threads;
}

QVariantMap PerformanceModel::memory() const
{
    const qulonglong swapUsed = m_mem.swapTotal > m_mem.swapFree ? m_mem.swapTotal - m_mem.swapFree : 0;
    return {
        {QStringLiteral("total"), double(m_mem.total)},
        {QStringLiteral("inUse"), double(m_mem.inUse())},
        {QStringLiteral("available"), double(m_mem.available)},
        {QStringLiteral("cached"), double(m_mem.inCache())},
        {QStringLiteral("committed"), double(m_mem.committed)},
        {QStringLiteral("commitLimit"), double(m_mem.commitLimit)},
        {QStringLiteral("swapTotal"), double(m_mem.swapTotal)},
        {QStringLiteral("swapUsed"), double(swapUsed)},
    };
}

QVariantList PerformanceModel::memoryHistory() const
{
    return m_memSeries.toList();
}

QVariantList PerformanceModel::disks() const
{
    return m_disks;
}

QVariantList PerformanceModel::adapters() const
{
    return m_adapters;
}

QAbstractItemModel *PerformanceModel::diskList()
{
    return &m_diskList;
}

QAbstractItemModel *PerformanceModel::adapterList()
{
    return &m_adapterList;
}

QAbstractItemModel *PerformanceModel::gpuList()
{
    return &m_gpuList;
}

QVariantList PerformanceModel::gpus() const
{
    return m_gpus;
}

double PerformanceModel::diskPercent() const
{
    return m_diskPercent;
}

double PerformanceModel::networkBitsPerSecond() const
{
    return m_netBits;
}

} // namespace Raye::TaskManager
