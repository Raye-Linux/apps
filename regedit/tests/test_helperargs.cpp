// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "helperargs.h"
#include "locations.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <unistd.h>

using namespace Reg;

class TestHelperArgs : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void roundTrip()
    {
        const Change c{Change::Set, "/etc/xdg/kdeglobals", Format::KConfig, {"Containments", "1"}, "k", {}};
        QString error;
        const auto back = parseHelperArguments(helperArguments(c), "v a l", &error);
        QVERIFY2(back, qPrintable(error));
        QCOMPARE(back->group, c.group);
        QCOMPARE(back->text, QStringLiteral("v a l"));
        QCOMPARE(back->format, Format::KConfig);
        const auto top = parseHelperArguments({"remove", "/etc/default/grub", "-", "GRUB_TIMEOUT"}, {}, &error);
        QVERIFY(top);
        QVERIFY(top->group.isEmpty());
        QCOMPARE(top->op, Change::Remove);
        QCOMPARE(top->format, Format::Shell);
    }

    void refusals()
    {
        QString error;
        QVERIFY(!parseHelperArguments({"set", "/etc/shadow", "-", "root"}, "x", &error));
        QVERIFY(!parseHelperArguments({"write", "/etc/default/grub", "-", "k"}, "x", &error));
        QVERIFY(!parseHelperArguments({"set", "/etc/default/grub", "-"}, "x", &error));
        QVERIFY(!parseHelperArguments({"set", "/etc/hostname", "-", "k"}, "x", &error));
        QVERIFY(!parseHelperArguments({"set", "/etc/default/grub", "-", "k"}, QByteArray(MaxFileSize + 1, 'a'), &error));
    }

    void diskChecks()
    {
        // /etc is root's on any machine the tests run on, and the owner check is the one that varies
        QVERIFY(!refuseSystemPath("/etc/fstab").isEmpty());
        if (QFile::exists("/etc/default/grub"))
            QVERIFY(!refuseSystemPath("/etc/default/grub", ::getuid() + 1).isEmpty());
        QVERIFY(refuseSystemPath("/etc/default/grub-not-there").contains("not on the list"));
    }

    void linksAreRefused()
    {
        QTemporaryDir tmp;
        const QString root = tmp.path();
        QDir().mkpath(root + "/etc/default");
        QDir().mkpath(root + "/elsewhere");
        QFile f(root + "/etc/default/grub");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();
        QCOMPARE(refuseSystemPath("/etc/default/grub", ::getuid(), root), QString());
        QVERIFY(!refuseSystemPath("/etc/default/grub", ::getuid() + 1, root).isEmpty());

        QFile g(root + "/elsewhere/real");
        QVERIFY(g.open(QIODevice::WriteOnly));
        g.close();
        QVERIFY(QFile::link(root + "/elsewhere/real", root + "/etc/default/locale"));
        QVERIFY(refuseSystemPath("/etc/default/locale", ::getuid(), root).contains("a link"));

        // a folder on the way that is a link
        QDir().mkpath(root + "/elsewhere/systemd");
        QFile h(root + "/elsewhere/systemd/logind.conf");
        QVERIFY(h.open(QIODevice::WriteOnly));
        h.close();
        QVERIFY(QFile::link(root + "/elsewhere/systemd", root + "/etc/systemd"));
        QVERIFY(refuseSystemPath("/etc/systemd/logind.conf", ::getuid(), root).contains("on the way"));
    }

    // keys that make root load or run something the user chose
    void codeKeysAreRefused()
    {
        QString error;
        const auto set = [&](const char *path, const QString &group, const char *key) {
            return parseHelperArguments({"set", path, group, key}, "x", &error).has_value();
        };
        QVERIFY(!set("/etc/systemd/system.conf", "Manager", "DefaultEnvironment"));
        QVERIFY(error.contains("DefaultEnvironment"));
        QVERIFY(!set("/etc/systemd/system.conf.d/x.conf", "Manager", "ManagerEnvironment"));
        QVERIFY(!set("/etc/default/grub", "-", "GRUB_CMDLINE_LINUX"));
        QVERIFY(!set("/etc/default/grub", "-", "GRUB_CMDLINE_LINUX_DEFAULT"));
        QVERIFY(!set("/etc/sddm.conf", "X11", "DisplayCommand"));
        QVERIFY(!set("/etc/sddm.conf.d/10-raye.conf", "Wayland", "CompositorCommand"));
        QVERIFY(!set("/etc/sddm.conf", "X11", "ServerPath"));
        // LD_PRELOAD here runs in the greeter, which sees every login password
        QVERIFY(!set("/etc/sddm.conf", "General", "GreeterEnvironment"));
        QVERIFY(!set("/etc/sddm.conf.d/10-raye.conf", "General", "GreeterEnvironment"));
        QVERIFY(!set("/etc/snapper/configs/root", "-", "ALLOW_USERS"));
        QVERIFY(!parseHelperArguments({"remove", "/etc/default/grub", "-", "GRUB_CMDLINE_LINUX"}, {}, &error));
        // the everyday ones still go through
        QVERIFY2(set("/etc/default/grub", "-", "GRUB_TIMEOUT"), qPrintable(error));
        QVERIFY2(set("/etc/systemd/logind.conf", "Login", "HandleLidSwitch"), qPrintable(error));
        QVERIFY2(set("/etc/sddm.conf", "Theme", "Current"), qPrintable(error));
        QVERIFY2(set("/etc/snapper/configs/root", "-", "TIMELINE_LIMIT_DAILY"), qPrintable(error));
    }

    void environmentIsReadOnly()
    {
        QString error;
        QVERIFY(!parseHelperArguments({"set", "/etc/environment", "-", "EDITOR"}, "nano", &error));
    }

    // dpkg keeps the old file on the next update and the new defaults never arrive
    void packageFilesAreRefused()
    {
        QTemporaryDir tmp;
        const QString root = tmp.path();
        QDir().mkpath(root + "/etc/xdg");
        QDir().mkpath(root + "/var/lib/dpkg/info");
        for (const char *name : {"/etc/xdg/kwinrc", "/etc/xdg/mine"}) {
            QFile f(root + name);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("[General]\nx=1\n");
        }
        QFile list(root + "/var/lib/dpkg/info/raye-theme.conffiles");
        QVERIFY(list.open(QIODevice::WriteOnly));
        list.write("/etc/xdg/kdeglobals\n/etc/xdg/kwinrc\n");
        list.close();
        QCOMPARE(packageOf("/etc/xdg/kwinrc", root), QStringLiteral("raye-theme"));
        QCOMPARE(packageOf("/etc/xdg/mine", root), QString());
        QVERIFY(refuseSystemPath("/etc/xdg/kwinrc", ::getuid(), root).contains("raye-theme"));
        QCOMPARE(refuseSystemPath("/etc/xdg/mine", ::getuid(), root), QString());
    }

    // only what the editor shows: no dpkg leftovers, nothing that is not a settings file
    void onlyWhatTheEditorShows()
    {
        QVERIFY(!systemWritable("/etc/xdg/kwinrc.dpkg-dist"));
        QVERIFY(!systemWritable("/etc/xdg/kwinrc.dpkg-old"));
        QVERIFY(!systemWritable("/etc/sddm.conf.d/10-raye.conf.ucf-dist"));
        QTemporaryDir tmp;
        const QString root = tmp.path();
        QDir().mkpath(root + "/etc/xdg");
        QFile f(root + "/etc/xdg/data.json");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{\"a\": 1}");
        f.close();
        QVERIFY(refuseSystemPath("/etc/xdg/data.json", ::getuid(), root).contains("settings"));
    }

    // the editor lists /etc/xdg without empty files or skipped folders
    void xdgFilesTheEditorHides()
    {
        QTemporaryDir tmp;
        const QString root = tmp.path();
        QDir().mkpath(root + "/etc/xdg/pulse");
        QDir().mkpath(root + "/etc/sddm.conf.d");
        const auto write = [&](const char *name, const QByteArray &text) {
            QFile f(root + name);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(text);
        };
        write("/etc/xdg/emptyrc", {});
        write("/etc/xdg/pulse/client.conf", "autospawn=no\n");
        write("/etc/xdg/kwinrc", "[General]\nx=1\n");
        write("/etc/sddm.conf.d/20-empty.conf", {});
        QVERIFY(!Reg::userFiles(root + "/etc/xdg", 2).contains("emptyrc"));
        QVERIFY(!refuseSystemPath("/etc/xdg/emptyrc", ::getuid(), root).isEmpty());
        QVERIFY(!refuseSystemPath("/etc/xdg/pulse/client.conf", ::getuid(), root).isEmpty());
        QCOMPARE(refuseSystemPath("/etc/xdg/kwinrc", ::getuid(), root), QString());
        // System lists empty drop-ins, so those stay writable
        QVERIFY(Reg::systemFiles(root).contains(root + "/etc/sddm.conf.d/20-empty.conf"));
        QCOMPARE(refuseSystemPath("/etc/sddm.conf.d/20-empty.conf", ::getuid(), root), QString());
    }
};

QTEST_GUILESS_MAIN(TestHelperArgs)
#include "test_helperargs.moc"
