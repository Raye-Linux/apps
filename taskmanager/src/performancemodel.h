// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The Performance page: one reading a second of the processor, the
// memory, each disk, each network adapter and each graphics card, and the
// last minute of each kept for the graphs. The readings are in sysinfo.h.

#pragma once

#include "devicelist.h"
#include "sysinfo.h"

#include <QHash>
#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <QVariantList>

namespace Raye::TaskManager
{

class PerformanceModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(int historyLength READ historyLength CONSTANT)

    Q_PROPERTY(double cpuPercent READ cpuPercent NOTIFY sampled)
    Q_PROPERTY(QVariantList cpuHistory READ cpuHistory NOTIFY sampled)
    Q_PROPERTY(int cpuMhz READ cpuMhz NOTIFY sampled)
    Q_PROPERTY(QVariantMap cpuInfo READ cpuInfo CONSTANT)
    Q_PROPERTY(qulonglong uptimeSeconds READ uptimeSeconds NOTIFY sampled)
    Q_PROPERTY(int threadCount READ threadCount NOTIFY sampled)

    // total, inUse, available, cached, committed, commitLimit, swapTotal, swapUsed
    Q_PROPERTY(QVariantMap memory READ memory NOTIFY sampled)
    Q_PROPERTY(QVariantList memoryHistory READ memoryHistory NOTIFY sampled)

    // one map per disk, network adapter and graphics card; each has a
    // "history" list of the same length as cpuHistory
    Q_PROPERTY(QVariantList disks READ disks NOTIFY sampled)
    Q_PROPERTY(QVariantList adapters READ adapters NOTIFY sampled)
    Q_PROPERTY(QVariantList gpus READ gpus NOTIFY sampled)
    // the same three as list models, for the tiles: rows change in place
    Q_PROPERTY(QAbstractItemModel *diskList READ diskList CONSTANT)
    Q_PROPERTY(QAbstractItemModel *adapterList READ adapterList CONSTANT)
    Q_PROPERTY(QAbstractItemModel *gpuList READ gpuList CONSTANT)

    // the whole machine's disk and network now, for the Processes headings
    Q_PROPERTY(double diskPercent READ diskPercent NOTIFY sampled)
    Q_PROPERTY(double networkBitsPerSecond READ networkBitsPerSecond NOTIFY sampled)

public:
    explicit PerformanceModel(QObject *parent = nullptr);

    bool active() const;
    void setActive(bool active);
    int historyLength() const;

    double cpuPercent() const;
    QVariantList cpuHistory() const;
    int cpuMhz() const;
    QVariantMap cpuInfo() const;
    qulonglong uptimeSeconds() const;
    int threadCount() const;
    QVariantMap memory() const;
    QVariantList memoryHistory() const;
    QVariantList disks() const;
    QVariantList adapters() const;
    QVariantList gpus() const;
    QAbstractItemModel *diskList();
    QAbstractItemModel *adapterList();
    QAbstractItemModel *gpuList();
    double diskPercent() const;
    double networkBitsPerSecond() const;

    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void activeChanged();
    void sampled();

private:
    struct Series {
        QList<double> values;
        void push(double v, int length);
        QVariantList toList() const;
    };

    bool m_active = true;
    QTimer m_timer;
    qint64 m_lastAt = 0;
    CpuInfo m_cpuInfo;
    QByteArray m_pciIds;
    bool m_pciIdsRead = false;

    CpuTimes m_cpuBefore;
    double m_cpu = 0.0;
    Series m_cpuSeries;
    int m_mhz = 0;
    qulonglong m_uptime = 0;
    int m_threads = 0;
    MemInfo m_mem;
    Series m_memSeries;

    QHash<QString, DiskCounters> m_diskBefore;
    QHash<QString, NetCounters> m_netBefore;
    QHash<QString, Series> m_series;
    QHash<QString, DiskInfo> m_diskInfo;
    // adapter -> its IPv4 and IPv6, read again every few seconds
    QHash<QString, QStringList> m_addresses;
    int m_addressAge = 0;
    QVariantList m_disks;
    QVariantList m_adapters;
    QVariantList m_gpus;
    DeviceList m_diskList{this};
    DeviceList m_adapterList{this};
    DeviceList m_gpuList{this};
    double m_diskPercent = 0.0;
    double m_netBits = 0.0;
};

} // namespace Raye::TaskManager
