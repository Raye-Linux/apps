// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "launcher.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace {
void write(const QString &path, const QByteArray &text = {})
{
    QDir().mkpath(path.section(QLatin1Char('/'), 0, -2));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(text);
}
} // namespace

class TestLauncher : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void readsPagesOfTheLayout()
    {
        const QByteArray layout = R"({"version": 2, "categories": [
            {"id": "system", "pages": [{"id": "system/display"}, {"id": "system/sound"}]},
            {"id": "apps", "pages": [{"id": "apps/installed"}]}]})";
        const QSet<QString> pages = Launcher::pagesOf(layout);
        QCOMPARE(pages, (QSet<QString>{"system", "system/display", "system/sound", "apps", "apps/installed"}));
        QVERIFY(Launcher::pagesOf("not json").isEmpty());
    }

    void readsWhatPagesNeed()
    {
        const QByteArray layout = R"({"categories": [
            {"id": "network", "requires": "wifi", "pages": [{"id": "network/wifi", "requires": "wifi"},
                                                            {"id": "network/vpn"}]}]})";
        const QHash<QString, QString> needs = Launcher::requirementsOf(layout);
        QCOMPARE(needs.value("network/wifi"), QString("wifi"));
        // a category that is bare without wifi still has its other pages
        QVERIFY(!needs.contains("network"));
        QVERIFY(!needs.contains("network/vpn"));
    }

    void opensSettingsOnThePage()
    {
        QCOMPARE(Launcher::settingsArguments("system/sound"), (QStringList{"--page", "system/sound"}));
    }

    void probesTheHardware()
    {
        QTemporaryDir root;
        const QString r = root.path();
        QVERIFY(!Launcher::probe(r, "wifi"));
        QVERIFY(!Launcher::probe(r, "bluetooth"));
        QVERIFY(!Launcher::probe(r, "touchpad"));
        QVERIFY(!Launcher::probe(r, "firewall"));
        QVERIFY(!Launcher::probe(r, "raye-account"));

        write(r + "/sys/class/net/eth0/address");
        QVERIFY(!Launcher::probe(r, "wifi"));
        write(r + "/sys/class/net/wlp2s0/phy80211/name");
        QVERIFY(Launcher::probe(r, "wifi"));

        write(r + "/sys/class/bluetooth/hci0/name");
        QVERIFY(Launcher::probe(r, "bluetooth"));

        write(r + "/sys/class/input/input3/name", "OpenRGB Keyboard\n");
        QVERIFY(!Launcher::probe(r, "tablet"));
        write(r + "/sys/class/input/input4/name", "SYNA2B52:00 06CB:7A13 Touchpad\n");
        QVERIFY(Launcher::probe(r, "touchpad"));

        QVERIFY(!Launcher::probe(r, "online-accounts"));
        write(r + "/usr/share/accounts/providers/google.provider");
        QVERIFY(Launcher::probe(r, "online-accounts"));

        write(r + "/etc/ufw/ufw.conf", "ENABLED=no\n");
        QVERIFY(Launcher::probe(r, "firewall"));

        // anything else is for Settings to decide
        QVERIFY(Launcher::probe(r, "something-new"));
        QVERIFY(Launcher::probe(r, "wifi|bluetooth"));
        QVERIFY(!Launcher::probe(r, "tablet|raye-account"));
    }

    void hidesPagesSettingsDoesNotHave()
    {
        qputenv("RAYE_SETTINGS_LAYOUT", QFINDTESTDATA("layout-sample.json").toUtf8());
        Launcher l;
        QVERIFY(l.hasPage("system/display"));
        QVERIFY(!l.hasPage("system/nope"));
        qputenv("RAYE_SETTINGS_LAYOUT", "/nonexistent/layout.json");
        Launcher unknown;
        QVERIFY(unknown.hasPage("system/nope"));
        QVERIFY(unknown.hasProgram("sh"));
        QVERIFY(!unknown.hasProgram("raye-no-such-program"));
    }

    void hidesPagesThisPcCannotUse()
    {
        QTemporaryDir root;
        qputenv("RAYE_SYSTEM_ROOT", root.path().toUtf8());
        qputenv("RAYE_SETTINGS_LAYOUT", QFINDTESTDATA("layout-sample.json").toUtf8());
        Launcher wired;
        QVERIFY(wired.hasPage("network/ethernet"));
        QVERIFY(!wired.hasPage("network/wifi"));
        QVERIFY(!wired.has("wifi"));
        write(root.path() + "/sys/class/net/wlan0/wireless/x");
        Launcher wireless;
        QVERIFY(wireless.hasPage("network/wifi"));
        qunsetenv("RAYE_SYSTEM_ROOT");
    }

    // The views ask the same questions for every item, again at each key
    // typed in the search: the files are read once.
    void answersOncePerRun()
    {
        QTemporaryDir root;
        write(root.path() + "/sys/class/input/input7/name", "SynPS/2 Synaptics TouchPad\n");
        qputenv("RAYE_SYSTEM_ROOT", root.path().toUtf8());
        Launcher l;
        QVERIFY(l.has("touchpad"));
        QVERIFY(QDir(root.path() + "/sys").removeRecursively());
        QVERIFY(l.has("touchpad"));
        qunsetenv("RAYE_SYSTEM_ROOT");
    }

    // A click starts the program and comes back at once; the pointer is
    // the busy one until the program shows up, or at most busyTimeout.
    void startsDetachedWithFeedback()
    {
        QTemporaryDir bin;
        const QString said = bin.path() + "/said";
        write(bin.path() + "/raye-settings", "#!/bin/sh\necho \"$@\" > '" + said.toUtf8() + "'\nsleep 3\n");
        QFile::setPermissions(bin.path() + "/raye-settings", QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        const QByteArray path = qgetenv("PATH");
        qputenv("PATH", QByteArray(bin.path().toUtf8() + ":" + path));

        Launcher l;
        l.setProperty("busyTimeout", 400);
        QSignalSpy busy(&l, SIGNAL(busyChanged()));
        QElapsedTimer t;
        t.start();
        l.openPage("system/sound");
        // the program runs 3 s: well under that means openPage did not wait
        // for it. Not 150 ms: a busy build machine took 474 ms on 2026-11-19
        QVERIFY2(t.elapsed() < 1500, qPrintable(QString::number(t.elapsed())));
        QVERIFY(l.property("busy").toBool());
        auto heard = [&said] {
            QFile f(said);
            return f.open(QIODevice::ReadOnly) ? f.readAll().trimmed() : QByteArray();
        };
        QTRY_COMPARE(heard(), QByteArray("--page system/sound"));
        // still running, the window never came: the pointer comes back anyway
        QTRY_VERIFY(!l.property("busy").toBool());
        QVERIFY(busy.count() >= 2);
        qputenv("PATH", path);
    }

    void picksTheFirstProgramThatIsThere()
    {
        Launcher l;
        QCOMPARE(l.firstProgram({"raye-no-such-program", "sh", "true"}), QString("sh"));
        QCOMPARE(l.firstProgram({"raye-no-such-program"}), QString());
        QCOMPARE(l.firstProgram({}), QString());
    }
};

QTEST_MAIN(TestLauncher)
#include "test_launcher.moc"
