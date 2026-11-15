// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Runs in a session bus of its own with XDG_CONFIG_HOME in a temporary
// folder (see CMakeLists.txt), so the writes go to a database of its own.

#include "dconfstore.h"

#include <QTest>

using namespace Reg;

class TestDconf : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typesAreKept()
    {
        QCOMPARE(*dconfCanonical("true", "b"), QStringLiteral("true"));
        QVERIFY(!dconfCanonical("'true'", "b"));
        QVERIFY(!dconfCanonical("yes", "b"));
        QCOMPARE(*dconfCanonical("12", "u"), QStringLiteral("12"));
        QVERIFY(!dconfCanonical("-1", "u"));
        QCOMPARE(*dconfCanonical("['a','b']", "as"), QStringLiteral("['a', 'b']"));
        QString error;
        QVERIFY(!dconfCanonical("[1]", "as", &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!dconfCanonical("x", "not a type"));
        QCOMPARE(*dconfGuessType("['a']"), QStringLiteral("as"));
        QCOMPARE(*dconfGuessType("5"), QStringLiteral("i"));
        QVERIFY(!dconfGuessType("not gvariant"));
        QCOMPARE(dconfQuote("it's"), QStringLiteral("\"it's\""));
        QCOMPARE(dconfDisplay({"s", "'Inter 10'"}), QStringLiteral("Inter 10"));
    }

    void newValues()
    {
        QCOMPARE(dconfNewValue("hello", "Text")->text, QStringLiteral("'hello'"));
        QCOMPARE(dconfNewValue("5", "Number")->type, QStringLiteral("i"));
        QCOMPARE(dconfNewValue("1.5", "Number")->type, QStringLiteral("d"));
        QVERIFY(!dconfNewValue("five", "Number"));
        QCOMPARE(dconfNewValue("false", "YesNo")->text, QStringLiteral("false"));
        QCOMPARE(dconfNewValue("['x']", "List")->type, QStringLiteral("as"));
    }

    void writeReadReset()
    {
        QVERIFY2(qEnvironmentVariableIsSet("DBUS_SESSION_BUS_ADDRESS"), "run me in dbus-run-session");
        QVERIFY2(qEnvironmentVariable("XDG_CONFIG_HOME").contains("tmp"), "run me with a temporary XDG_CONFIG_HOME");
        DconfStore store;
        QString error;
        QVERIFY(store.writable("/org/raye/test/flag"));
        QVERIFY2(store.write("/org/raye/test/flag", {"b", "true"}, &error), qPrintable(error));
        QVERIFY2(store.write("/org/raye/test/name", {"s", "'Raye'"}, &error), qPrintable(error));
        QVERIFY(!store.write("/org/raye/test/bad", {"b", "'no'"}, &error));
        QVERIFY(!store.write("not/a/key", {"b", "true"}, &error));
        QCOMPARE(store.read("/org/raye/test/flag")->type, QStringLiteral("b"));
        QCOMPARE(store.read("/org/raye/test/name")->text, QStringLiteral("'Raye'"));
        QCOMPARE(store.list("/org/raye/"), QStringList{"test/"});
        QCOMPARE(store.list("/org/raye/test/"), (QStringList{"flag", "name"}));
        QVERIFY(store.reset("/org/raye/test/flag", &error));
        QVERIFY(!store.read("/org/raye/test/flag"));
        QVERIFY(!store.read("/org/raye/test/bad"));
    }
};

QTEST_GUILESS_MAIN(TestDconf)
#include "test_dconf.moc"
