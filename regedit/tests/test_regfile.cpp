// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "regfile.h"

#include <QTest>

using namespace Reg;

class TestRegFile : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void roundTrip()
    {
        const QList<RegSection> in{
            {"HKEY_CURRENT_USER\\Config\\kdeglobals\\General",
             {{"AccentColor", false, false, "123,60,255"}, {"Quote \"x\"", false, false, "a\\b\nc"}, {"Old", true, false, {}}}},
            {"HKEY_CURRENT_USER\\Desktop\\org\\gnome\\desktop\\interface", {{"color-scheme", false, true, "'prefer-dark'"}}},
        };
        const QString text = writeRegFile(in);
        QVERIFY(text.startsWith("Raye Registry Editor 1\n\n[HKEY_CURRENT_USER\\Config\\kdeglobals\\General]\n\"AccentColor\"=\"123,60,255\"\n"));
        QVERIFY(text.contains("\"Old\"=-\n"));
        QVERIFY(text.contains("\"color-scheme\"=gvariant:'prefer-dark'\n"));
        const auto out = readRegFile(text);
        QVERIFY(out);
        QCOMPARE(out->size(), 2);
        QCOMPARE(out->at(0).path, in.at(0).path);
        QCOMPARE(out->at(0).items.at(1).name, QStringLiteral("Quote \"x\""));
        QCOMPARE(out->at(0).items.at(1).text, QStringLiteral("a\\b\nc"));
        QVERIFY(out->at(0).items.at(2).remove);
        QVERIFY(out->at(1).items.at(0).gvariant);
        QCOMPARE(out->at(1).items.at(0).text, QStringLiteral("'prefer-dark'"));
    }

    void refusesWhatItCannotRead()
    {
        QString error;
        int line = 0;
        QVERIFY(!readRegFile("Windows Registry Editor Version 5.00\n", &error, &line));
        QCOMPARE(line, 1);
        QVERIFY(!readRegFile("Raye Registry Editor 1\n\"a\"=\"b\"\n", &error, &line));
        QCOMPARE(line, 2);
        QVERIFY(!readRegFile("Raye Registry Editor 1\n[K]\n\"a\"=\"b\n", &error, &line));
        QCOMPARE(line, 3);
        QVERIFY(!readRegFile("Raye Registry Editor 1\n[K]\n\"a\"=\"b\" junk\n", &error, &line));
        QVERIFY(readRegFile("Raye Registry Editor 1\r\n\r\n; comment\r\n[K]\r\n\"a\"=\"b\"\r\n"));
    }
};

QTEST_GUILESS_MAIN(TestRegFile)
#include "test_regfile.moc"
