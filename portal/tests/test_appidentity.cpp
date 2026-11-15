// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "raye/appidentity.h"

#include <QTest>

using namespace Raye;
using namespace Qt::StringLiterals;

class TestAppIdentity : public QObject
{
    Q_OBJECT

    QStringList dirs() const
    {
        return {QStringLiteral(FIXTURES "/applications"), QStringLiteral(FIXTURES "/more")};
    }

private Q_SLOTS:
    void knownApp()
    {
        const AppIdentity app = appIdentity(u"com.discordapp.Discord"_s, dirs());
        QVERIFY(app.known);
        QCOMPARE(app.name, u"Discord"_s);
        QCOMPARE(app.iconName, u"com.discordapp.Discord"_s);
        QCOMPARE(app.id, u"com.discordapp.Discord"_s);
    }

    void firstFolderWins()
    {
        const AppIdentity app = appIdentity(u"com.discordapp.Discord"_s, {QStringLiteral(FIXTURES "/more"), QStringLiteral(FIXTURES "/applications")});
        QCOMPARE(app.name, u"Discord (second folder)"_s);
        // no Icon= line: the generic one
        QCOMPARE(app.iconName, u"application-x-executable"_s);
    }

    void fullNameFirst()
    {
        const AppIdentity app = appIdentity(u"org.example.FullName"_s, dirs());
        QCOMPARE(app.name, u"Example Full Name"_s);
        QCOMPARE(app.iconName, u"/opt/example/icon.png"_s);
    }

    void nameWithSemicolon()
    {
        QCOMPARE(appIdentity(u"org.example.NoIcon"_s, dirs()).name, u"No Icon; Really"_s);
    }

    void unknownApps_data()
    {
        QTest::addColumn<QString>("appId");
        QTest::newRow("empty") << QString();
        QTest::newRow("missing") << u"org.example.Missing"_s;
        QTest::newRow("empty name") << u"org.example.NoName"_s;
        QTest::newRow("not an app") << u"org.example.Link"_s;
        QTest::newRow("path") << u"../applications/com.discordapp.Discord"_s;
        QTest::newRow("absolute") << u"/etc/passwd"_s;
        QTest::newRow("dots") << u"com..discordapp"_s;
        QTest::newRow("hidden") << u".Discord"_s;
        QTest::newRow("space") << u"com discordapp"_s;
    }
    void unknownApps()
    {
        QFETCH(QString, appId);
        const AppIdentity app = appIdentity(appId, dirs());
        QVERIFY(!app.known);
        QCOMPARE(app.name, u"An app"_s);
        QCOMPARE(app.iconName, u"application-x-executable"_s);
    }

    void plausibleIds()
    {
        QVERIFY(isPlausibleAppId(u"org.mozilla.firefox"_s));
        QVERIFY(isPlausibleAppId(u"firefox"_s));
        QVERIFY(isPlausibleAppId(u"org.kde.konsole-2"_s));
        QVERIFY(!isPlausibleAppId(u""_s));
        QVERIFY(!isPlausibleAppId(u"a/b"_s));
        QVERIFY(!isPlausibleAppId(u"a..b"_s));
        QVERIFY(!isPlausibleAppId(QString(300, u'a')));
    }
};

QTEST_GUILESS_MAIN(TestAppIdentity)
#include "test_appidentity.moc"
