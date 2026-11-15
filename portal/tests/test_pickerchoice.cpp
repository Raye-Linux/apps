// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "raye/pickerchoice.h"

#include <QTest>

using namespace Raye;
using namespace Qt::StringLiterals;

class TestPickerChoice : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void rayeWhenBuilt()
    {
        const PickerChoice choice = choosePicker(true, QString(), QByteArray());
        QVERIFY(choice.raye);
        QCOMPARE(choice.module, u"org.raye.portal.picker"_s);
        QCOMPARE(choice.component, u"SharePicker"_s);
        QCOMPARE(choice.domain, u"xdg-desktop-portal-raye"_s);
    }

    void desktopWhenNotBuilt()
    {
        const PickerChoice choice = choosePicker(false, QString(), QByteArray());
        QVERIFY(!choice.raye);
        QCOMPARE(choice.module, u"org.kde.xdgdesktopportal"_s);
        QCOMPARE(choice.component, u"ScreenChooserDialog"_s);
        QCOMPARE(choice.domain, u"xdg-desktop-portal-kde"_s);
    }

    void userAsksForTheDesktopChooser()
    {
        QVERIFY(!choosePicker(true, u"kde"_s, QByteArray()).raye);
        QVERIFY(!choosePicker(true, u" KDE "_s, QByteArray()).raye);
        QVERIFY(!choosePicker(true, QString(), "kde").raye);
        // anything else keeps the Raye picker
        QVERIFY(choosePicker(true, u"raye"_s, QByteArray()).raye);
        QVERIFY(choosePicker(true, u"something"_s, "other").raye);
    }

    void cursorMode_data()
    {
        QTest::addColumn<uint>("asked");
        QTest::addColumn<QVariant>("showPointer");
        QTest::addColumn<uint>("expected");
        QTest::newRow("no toggle, embedded") << 2u << QVariant() << 2u;
        QTest::newRow("no toggle, metadata") << 4u << QVariant() << 4u;
        QTest::newRow("on, embedded") << 2u << QVariant(true) << 2u;
        QTest::newRow("off, embedded") << 2u << QVariant(false) << 1u;
        QTest::newRow("off, metadata") << 4u << QVariant(false) << 1u;
        QTest::newRow("hidden stays hidden") << 1u << QVariant(true) << 1u;
        QTest::newRow("unset stays unset") << 0u << QVariant(false) << 0u;
    }
    void cursorMode()
    {
        QFETCH(uint, asked);
        QFETCH(QVariant, showPointer);
        QFETCH(uint, expected);
        QCOMPARE(effectiveCursorMode(asked, showPointer), expected);
    }
};

QTEST_GUILESS_MAIN(TestPickerChoice)
#include "test_pickerchoice.moc"
