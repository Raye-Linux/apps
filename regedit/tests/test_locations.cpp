// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "locations.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace Reg;

class TestLocations : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void helperList()
    {
        for (const char *ok : {"/etc/default/grub", "/etc/systemd/logind.conf", "/etc/sddm.conf.d/10-raye.conf",
                               "/etc/systemd/logind.conf.d/lid.conf", "/etc/snapper/configs/root", "/etc/xdg/kdeglobals",
                               "/etc/xdg/kcm-about-distrorc", "/etc/NetworkManager/conf.d/dns.conf"})
            QVERIFY2(systemWritable(QString::fromLatin1(ok)), ok);
        for (const char *no : {"/etc/fstab", "/etc/sudoers", "/etc/shadow", "/etc/passwd", "/etc/os-release",
                               "/etc/hostname", "/etc/default/../shadow", "/etc/xdg/../shadow", "etc/default/grub",
                               "/etc/xdg/autostart/x.desktop", "/etc/NetworkManager/system-connections/home.nmconnection",
                               "/etc/sddm.conf.d/x.sh", "/etc/xdg/a/b/c", "/etc/pam.d/common-auth", "/etc/default/grub/",
                               "/etc/systemd/system/x.service", "/etc//default/grub", "/etc/environment",
                               "/etc/xdg/kdeglobals.dpkg-dist"})
            QVERIFY2(!systemWritable(QString::fromLatin1(no)), no);
    }

    void formats()
    {
        QCOMPARE(formatFor("/home/a/.config/kwinrc"), Format::KConfig);
        QCOMPARE(formatFor("/home/a/.config/kdeglobals"), Format::KConfig);
        QCOMPARE(formatFor("/home/a/.config/gtk-3.0/settings.ini"), Format::Ini);
        QCOMPARE(formatFor("/etc/xdg/kdeglobals"), Format::KConfig);
        QCOMPARE(formatFor("/etc/default/grub"), Format::Shell);
        QCOMPARE(formatFor("/etc/snapper/configs/root"), Format::Shell);
        QCOMPARE(formatFor("/etc/systemd/logind.conf"), Format::Ini);
        QCOMPARE(formatFor("/etc/hostname"), Format::Plain);
    }

    void whatLooksLikeSettings()
    {
        QVERIFY(looksLikeSettings("# c\n\n[General]\n"));
        QVERIFY(looksLikeSettings("key=value\n"));
        QVERIFY(!looksLikeSettings("{\"a\": 1}"));
        QVERIFY(!looksLikeSettings("<?xml version=\"1.0\"?>"));
        QVERIFY(!looksLikeSettings(QByteArray("[A]\0\1", 5)));
        QVERIFY(!looksLikeSettings("just words\n"));
    }

    void userFilesSkipWhatIsNotSettings()
    {
        QTemporaryDir tmp;
        QDir d(tmp.path());
        auto put = [&](const QString &rel, const QByteArray &data) {
            d.mkpath(QFileInfo(tmp.filePath(rel)).path());
            QFile f(tmp.filePath(rel));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(data);
        };
        put("kdeglobals", "[General]\nx=1\n");
        put("gtk-3.0/settings.ini", "[Settings]\ngtk-theme-name=Breeze\n");
        put("app.json", "{}");
        put("dconf/user", "[x]\n");
        put("chromium/Local State", "a=1\n");
        put("a/b/c/deep.conf", "[A]\n");
        put("empty.conf", "");
        put("kwinrc.dpkg-old", "[General]\nx=1\n");
        put("kwinrc.ucf-dist", "[General]\nx=1\n");
        const QStringList want{"gtk-3.0/settings.ini", "kdeglobals"};
        QCOMPARE(userFiles(tmp.path()), want);
    }

    void systemFilesUnderARoot()
    {
        QTemporaryDir tmp;
        QDir(tmp.path()).mkpath("etc/default");
        QDir(tmp.path()).mkpath("etc/sddm.conf.d");
        for (const char *p : {"etc/default/grub", "etc/environment", "etc/sddm.conf.d/10-raye.conf", "etc/sddm.conf.d/README",
                              "etc/sddm.conf.d/10-raye.conf.dpkg-old", "etc/fstab"}) {
            QFile f(tmp.filePath(QString::fromLatin1(p)));
            QVERIFY(f.open(QIODevice::WriteOnly));
        }
        // /etc/environment is shown but never written
        const QStringList want{tmp.filePath("etc/default/grub"), tmp.filePath("etc/environment"),
                               tmp.filePath("etc/sddm.conf.d/10-raye.conf")};
        QCOMPARE(systemFiles(tmp.path()), want);
    }
};

QTEST_GUILESS_MAIN(TestLocations)
#include "test_locations.moc"
