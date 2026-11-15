// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Startup apps: what is listed, and that turning one off never writes in
// the system folder.

#include "autostart.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace Raye::TaskManager;

namespace
{
void write(const QString &path, const QByteArray &text)
{
    QDir().mkpath(path.left(path.lastIndexOf(QLatin1Char('/'))));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(text);
}

QByteArray read(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return {};
    }
    return f.readAll();
}

const AutostartEntry *find(const QList<AutostartEntry> &list, const QString &file)
{
    for (const AutostartEntry &e : list) {
        if (e.fileName == file) {
            return &e;
        }
    }
    return nullptr;
}
}

class TestAutostart : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void listsAndOverrides()
    {
        QTemporaryDir root;
        const QString sys = root.path() + QStringLiteral("/etc");
        const QString user = root.path() + QStringLiteral("/home");
        write(sys + QStringLiteral("/clock.desktop"), "[Desktop Entry]\nName=Clock\nName[fr]=Horloge\nExec=clock\n");
        write(sys + QStringLiteral("/gnomeonly.desktop"), "[Desktop Entry]\nName=G\nOnlyShowIn=GNOME;\n");
        write(sys + QStringLiteral("/notkde.desktop"), "[Desktop Entry]\nName=N\nNotShowIn=KDE;\n");
        write(sys + QStringLiteral("/gone.desktop"), "[Desktop Entry]\nName=Gone\nHidden=true\n");
        write(sys + QStringLiteral("/off.desktop"), "[Desktop Entry]\nName=Off\nX-GNOME-Autostart-enabled=false\n");
        write(user + QStringLiteral("/clock.desktop"), "[Desktop Entry]\nName=Clock\nExec=clock\nHidden=true\n");
        write(user + QStringLiteral("/mine.desktop"), "[Desktop Entry]\nName=Mine\nExec=mine\n");

        const QList<AutostartEntry> list = listAutostart({sys}, user, QStringLiteral("fr"), QStringLiteral("KDE"));
        QCOMPARE(list.size(), 3);
        const AutostartEntry *clock = find(list, QStringLiteral("clock.desktop"));
        QVERIFY(clock);
        QVERIFY(clock->systemEntry);
        QVERIFY(!clock->enabled);
        QCOMPARE(clock->path, user + QStringLiteral("/clock.desktop"));
        QVERIFY(find(list, QStringLiteral("mine.desktop"))->enabled);
        QVERIFY(!find(list, QStringLiteral("mine.desktop"))->systemEntry);
        QVERIFY(!find(list, QStringLiteral("off.desktop"))->enabled);
        QVERIFY(!find(list, QStringLiteral("gnomeonly.desktop")));
        QVERIFY(!find(list, QStringLiteral("gone.desktop")));
    }

    void turningOffCopiesIntoTheHome()
    {
        QTemporaryDir root;
        const QString sys = root.path() + QStringLiteral("/etc");
        const QString user = root.path() + QStringLiteral("/home");
        const QByteArray original = "[Desktop Entry]\nName=Clock\nExec=clock\n# a comment\n";
        write(sys + QStringLiteral("/clock.desktop"), original);
        const QByteArray before = QCryptographicHash::hash(read(sys + QStringLiteral("/clock.desktop")), QCryptographicHash::Sha256);

        QList<AutostartEntry> list = listAutostart({sys}, user, QStringLiteral("en"), QStringLiteral("KDE"));
        QVERIFY(setAutostartEnabled(list.first(), false, user));
        list = listAutostart({sys}, user, QStringLiteral("en"), QStringLiteral("KDE"));
        QVERIFY(!list.first().enabled);
        QVERIFY(read(user + QStringLiteral("/clock.desktop")).contains("Hidden=true"));
        QVERIFY(read(user + QStringLiteral("/clock.desktop")).contains("# a comment"));

        QVERIFY(setAutostartEnabled(list.first(), true, user));
        list = listAutostart({sys}, user, QStringLiteral("en"), QStringLiteral("KDE"));
        QVERIFY(list.first().enabled);
        QVERIFY(!read(user + QStringLiteral("/clock.desktop")).contains("Hidden"));

        const QByteArray after = QCryptographicHash::hash(read(sys + QStringLiteral("/clock.desktop")), QCryptographicHash::Sha256);
        QCOMPARE(after, before);
    }

    void reEnablingClearsTheGnomeKeyToo()
    {
        QTemporaryDir root;
        const QString user = root.path() + QStringLiteral("/home");
        write(user + QStringLiteral("/x.desktop"), "[Desktop Entry]\nName=X\nX-GNOME-Autostart-enabled=false\n");
        QList<AutostartEntry> list = listAutostart({}, user, QStringLiteral("en"), QStringLiteral("KDE"));
        QVERIFY(!list.first().enabled);
        QVERIFY(setAutostartEnabled(list.first(), true, user));
        list = listAutostart({}, user, QStringLiteral("en"), QStringLiteral("KDE"));
        QVERIFY(list.first().enabled);
    }

    void unitNames()
    {
        QCOMPARE(autostartUnitName(QStringLiteral("org.kde.kalendarac.desktop")),
                 QStringLiteral("app-org.kde.kalendarac@autostart.service"));
        QCOMPARE(escapeUnitName(QStringLiteral("raye-update tray")), QStringLiteral("raye\\x2dupdate\\x20tray"));
        QCOMPARE(escapeUnitName(QStringLiteral(".hidden")), QStringLiteral("\\x2ehidden"));
    }

    void impact()
    {
        QCOMPARE(impactForCpuUsec(-1), Impact::NotMeasured);
        QCOMPARE(impactForCpuUsec(0), Impact::None);
        QCOMPARE(impactForCpuUsec(100000), Impact::Low);
        QCOMPARE(impactForCpuUsec(500000), Impact::Medium);
        QCOMPARE(impactForCpuUsec(5000000), Impact::High);
        QCOMPARE(parseCpuStatUsage("usage_usec 1234\nuser_usec 1000\n"), 1234);
        QCOMPARE(parseCpuStatUsage(""), -1);
    }
};

QTEST_GUILESS_MAIN(TestAutostart)
#include "test_autostart.moc"
