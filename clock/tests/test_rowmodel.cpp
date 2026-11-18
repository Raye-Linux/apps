// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rowmodel.h"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTest>

namespace
{
QVariantMap row(const char *key, int value = 0)
{
    return {{"key", QString::fromLatin1(key)}, {"value", value}};
}
} // namespace

class TestRowModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keeps_rows_that_stay()
    {
        RowModel m({"key", "value"});
        QAbstractItemModelTester tester(&m, QAbstractItemModelTester::FailureReportingMode::QtTest);
        m.setRows({row("a"), row("b"), row("c")});
        QCOMPARE(m.rowCount(), 3);

        QSignalSpy reset(&m, &QAbstractItemModel::modelReset);
        QSignalSpy inserted(&m, &QAbstractItemModel::rowsInserted);
        QSignalSpy removed(&m, &QAbstractItemModel::rowsRemoved);
        QSignalSpy changed(&m, &QAbstractItemModel::dataChanged);
        m.setRows({row("a"), row("c", 5), row("d")});
        QCOMPARE(reset.count(), 0);
        QCOMPARE(removed.count(), 1);
        QCOMPARE(inserted.count(), 1);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(m.data(m.index(1), Qt::UserRole + 1).toInt(), 5);
        QCOMPARE(m.get(2).value("key").toString(), QStringLiteral("d"));
    }

    void moves_in_place()
    {
        RowModel m({"key", "value"});
        QAbstractItemModelTester tester(&m, QAbstractItemModelTester::FailureReportingMode::QtTest);
        m.setRows({row("a"), row("b"), row("c")});
        QSignalSpy moved(&m, &QAbstractItemModel::rowsMoved);
        m.setRows({row("c"), row("a"), row("b")});
        QCOMPARE(moved.count(), 1);
        QCOMPARE(m.get(0).value("key").toString(), QStringLiteral("c"));
        QCOMPARE(m.get(2).value("key").toString(), QStringLiteral("b"));
        m.setRows({});
        QCOMPARE(m.rowCount(), 0);
        QCOMPARE(m.roleNames().value(Qt::UserRole), QByteArray("key"));
    }
};

QTEST_GUILESS_MAIN(TestRowModel)
#include "test_rowmodel.moc"
