// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingsfile.h"

#include <QTest>

using Reg::Format;
using Reg::SettingsFile;

class TestSettingsFile : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void kconfigGroups()
    {
        const auto f = SettingsFile::parse("top=1\n[General]\nName=Raye\n[Containments][1][General]\nplugin=org.kde.panel\n",
                                           Format::KConfig);
        const QList<QStringList> want{{}, {"General"}, {"Containments", "1", "General"}};
        QCOMPARE(f.groups(), want);
        QCOMPARE(f.entry({}, "top")->text, QStringLiteral("1"));
        QCOMPARE(f.entry({"Containments", "1", "General"}, "plugin")->text, QStringLiteral("org.kde.panel"));
    }

    void kconfigEscapes()
    {
        const auto f = SettingsFile::parse("[A]\nk=\\sa\\tb\\nc\\\\d\\s\n", Format::KConfig);
        QCOMPARE(f.entry({"A"}, "k")->text, QStringLiteral(" a\tb\nc\\d "));
        QCOMPARE(SettingsFile::escapeKConfig(QStringLiteral(" a\tb\nc\\d ")), QStringLiteral("\\sa\\tb\\nc\\\\d\\s"));
    }

    void kconfigLocks()
    {
        auto f = SettingsFile::parse("[A]\nfree=1\nfixed[$i]=2\nName[fr]=Bonjour\n[B][$i]\nx=1\n", Format::KConfig);
        QVERIFY(f.entry({"A"}, "fixed")->locked);
        QVERIFY(!f.entry({"A"}, "free")->locked);
        QCOMPARE(f.entry({"A"}, "Name[fr]")->text, QStringLiteral("Bonjour"));
        QVERIFY(f.groupLocked({"B"}));
        QString error;
        QVERIFY(!f.set({"A"}, "fixed", "3", &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!f.set({"B"}, "x", "2"));
        QVERIFY(!f.remove({"B"}, "x"));
        QVERIFY(f.set({"A"}, "free", "5"));

        const auto whole = SettingsFile::parse("[$i]\n[A]\nk=1\n", Format::KConfig);
        QVERIFY(whole.locked());
        auto copy = whole;
        QVERIFY(!copy.set({"A"}, "k", "2"));
    }

    void setKeepsEveryOtherLine()
    {
        const QByteArray before = "# made by hand\n[General]\n; old\nColorScheme=BreezeDark\n\n  Font = Inter,10\n[KDE]\nSingleClick=false\n";
        auto f = SettingsFile::parse(before, Format::KConfig);
        QVERIFY(f.set({"General"}, "ColorScheme", "RayeNight"));
        QCOMPARE(f.toBytes(),
                 QByteArray("# made by hand\n[General]\n; old\nColorScheme=RayeNight\n\n  Font = Inter,10\n[KDE]\nSingleClick=false\n"));
        QCOMPARE(f.entry({"General"}, "Font")->text, QStringLiteral("Inter,10"));
        // the spacing around = stays as the file had it
        QVERIFY(f.set({"General"}, "Font", "Inter,11"));
        QVERIFY(f.toBytes().contains("\n  Font = Inter,11\n"));
    }

    void unchangedIsByteForByte()
    {
        const QByteArray before = "[A]\r\nk=v\nweird line\n[B]\nx = 1";
        QCOMPARE(SettingsFile::parse(before, Format::KConfig).toBytes(), before);
        QCOMPARE(SettingsFile::parse(before, Format::Ini).toBytes(), before);
    }

    void newValueGoesAtTheEndOfItsGroup()
    {
        auto f = SettingsFile::parse("[A]\na=1\n\n[B]\nb=1", Format::KConfig);
        QVERIFY(f.set({"A"}, "c", "2"));
        QVERIFY(f.set({"C", "D"}, "e", "line\nbreak"));
        QVERIFY(f.set({}, "top", "x"));
        QCOMPARE(f.toBytes(), QByteArray("top=x\n[A]\na=1\nc=2\n\n[B]\nb=1\n\n[C][D]\ne=line\\nbreak\n"));
    }

    void removeTakesTheLine()
    {
        auto f = SettingsFile::parse("[A]\na=1\nb=2\n", Format::KConfig);
        QVERIFY(f.remove({"A"}, "a"));
        QCOMPARE(f.toBytes(), QByteArray("[A]\nb=2\n"));
        QString error;
        QVERIFY(!f.remove({"A"}, "nope", &error));
    }

    void iniIsRaw()
    {
        auto f = SettingsFile::parse("[Login]\n#HandleLidSwitch=suspend\nKillUserProcesses=no\nPath=C:\\x\n", Format::Ini);
        QCOMPARE(f.entry({"Login"}, "Path")->text, QStringLiteral("C:\\x"));
        QVERIFY(!f.entry({"Login"}, "#HandleLidSwitch"));
        QVERIFY(f.set({"Login"}, "HandleLidSwitch", "ignore"));
        QVERIFY(f.set({"Login"}, "Path", "a\\b"));
        QCOMPARE(f.toBytes(), QByteArray("[Login]\n#HandleLidSwitch=suspend\nKillUserProcesses=no\nPath=a\\b\nHandleLidSwitch=ignore\n"));
    }

    void shellQuotes()
    {
        auto f = SettingsFile::parse("# grub\nGRUB_TIMEOUT=5\nGRUB_CMDLINE_LINUX_DEFAULT=\"quiet splash\"\nexport LANG='fr_CA.UTF-8'\n",
                                     Format::Shell);
        QCOMPARE(f.groups(), QList<QStringList>{QStringList()});
        QCOMPARE(f.entry({}, "GRUB_CMDLINE_LINUX_DEFAULT")->text, QStringLiteral("quiet splash"));
        QCOMPARE(f.entry({}, "LANG")->text, QStringLiteral("fr_CA.UTF-8"));
        QVERIFY(f.set({}, "GRUB_CMDLINE_LINUX_DEFAULT", "quiet splash \"x\" $y"));
        QVERIFY(f.set({}, "LANG", "en_CA.UTF-8"));
        QVERIFY(f.set({}, "GRUB_TIMEOUT", "10"));
        QCOMPARE(f.toBytes(), QByteArray("# grub\nGRUB_TIMEOUT=10\nGRUB_CMDLINE_LINUX_DEFAULT=\"quiet splash \\\"x\\\" \\$y\"\n"
                                         "export LANG=en_CA.UTF-8\n"));
        QString error;
        QVERIFY(!f.set({"G"}, "k", "v", &error));
        QCOMPARE(SettingsFile::unquoteShell(QStringLiteral("\"a\\\"b\"")), QStringLiteral("a\"b"));
    }

    void plainIsReadOnly()
    {
        auto f = SettingsFile::parse("raye-test\n", Format::Plain);
        QCOMPARE(f.entry({}, QString())->text, QStringLiteral("raye-test"));
        QVERIFY(!f.set({}, QString(), "other"));
    }

    void oneLineValuesOnly()
    {
        auto f = SettingsFile::parse("[A]\n", Format::Ini);
        QString error;
        QVERIFY(!f.set({"A"}, "k", "two\nlines", &error));
        QVERIFY(!f.set({"A"}, "bad=key", "x", &error));
        QVERIFY(!f.set({"A"}, "", "x", &error));
    }
};

QTEST_GUILESS_MAIN(TestSettingsFile)
#include "test_settingsfile.moc"
