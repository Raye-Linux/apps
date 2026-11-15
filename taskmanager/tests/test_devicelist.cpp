// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The device tiles of the Performance page are made from this list. A new
// reading of the same disks must update their rows, not rebuild them: a
// rebuilt tile shows no graph for a frame, and the page blinks.

#include "devicelist.h"

#include <QSignalSpy>
#include <QTest>

using namespace Raye::TaskManager;

namespace
{
QVariantMap device(const QString &key, double active, const QVariantList &history = {})
{
    return {{QStringLiteral("key"), key}, {QStringLiteral("active"), active}, {QStringLiteral("history"), history}};
}

QVariantMap at(const DeviceList &list, int row)
{
    return list.data(list.index(row), DeviceList::DeviceRole).toMap();
}
}

class TestDeviceList : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void newReadingKeepsRows()
    {
        DeviceList list;
        list.setDevices({device(QStringLiteral("disk:sda"), 1), device(QStringLiteral("disk:sr0"), 0), device(QStringLiteral("net:eth0"), 0)});
        QSignalSpy reset(&list, &QAbstractItemModel::modelReset);
        QSignalSpy removed(&list, &QAbstractItemModel::rowsRemoved);
        QSignalSpy inserted(&list, &QAbstractItemModel::rowsInserted);
        QSignalSpy changed(&list, &QAbstractItemModel::dataChanged);

        list.setDevices({device(QStringLiteral("disk:sda"), 7, {1.0, 7.0}),
                         device(QStringLiteral("disk:sr0"), 0, {0.0, 0.0}),
                         device(QStringLiteral("net:eth0"), 0, {0.0, 3.0})});

        QCOMPARE(reset.count(), 0);
        QCOMPARE(removed.count(), 0);
        QCOMPARE(inserted.count(), 0);
        QCOMPARE(changed.count(), 3);
        QCOMPARE(list.rowCount(), 3);
        QCOMPARE(at(list, 0).value(QStringLiteral("active")).toDouble(), 7.0);
    }

    void sameReadingSaysNothing()
    {
        DeviceList list;
        const QVariantList devices = {device(QStringLiteral("disk:sda"), 2, {2.0})};
        list.setDevices(devices);
        QSignalSpy changed(&list, &QAbstractItemModel::dataChanged);
        list.setDevices(devices);
        QCOMPARE(changed.count(), 0);
    }

    void pluggedStickAddsOneRow()
    {
        DeviceList list;
        list.setDevices({device(QStringLiteral("disk:sda"), 0), device(QStringLiteral("net:eth0"), 0)});
        QSignalSpy reset(&list, &QAbstractItemModel::modelReset);
        QSignalSpy inserted(&list, &QAbstractItemModel::rowsInserted);
        list.setDevices({device(QStringLiteral("disk:sda"), 0), device(QStringLiteral("disk:sdb"), 0), device(QStringLiteral("net:eth0"), 0)});
        QCOMPARE(reset.count(), 0);
        QCOMPARE(inserted.count(), 1);
        QCOMPARE(at(list, 1).value(QStringLiteral("key")).toString(), QStringLiteral("disk:sdb"));

        QSignalSpy removed(&list, &QAbstractItemModel::rowsRemoved);
        list.setDevices({device(QStringLiteral("disk:sda"), 0), device(QStringLiteral("net:eth0"), 0)});
        QCOMPARE(removed.count(), 1);
        QCOMPARE(list.rowCount(), 2);
    }

    void qmlSeesTheDeviceRole()
    {
        DeviceList list;
        QCOMPARE(list.roleNames().value(DeviceList::DeviceRole), QByteArray("device"));
    }
};

QTEST_GUILESS_MAIN(TestDeviceList)
#include "test_devicelist.moc"
