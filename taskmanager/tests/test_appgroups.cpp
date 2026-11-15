// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Apps and their processes, read from cgroup names.

#include "appgroups.h"

#include <QTest>

using namespace Raye::TaskManager;

namespace
{
ProcEntry proc(int pid, const QString &name, const QString &app = {}, double cpu = 0, qlonglong mem = 0, bool autostart = false)
{
    ProcEntry e;
    e.pid = pid;
    e.name = name;
    e.appId = app;
    e.appName = app.isEmpty() ? QString() : app.section(QLatin1Char('.'), -1);
    e.cpu = cpu;
    e.memory = mem;
    e.autostart = autostart;
    return e;
}
}

class TestAppGroups : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scopeNames()
    {
        QCOMPARE(appUnitFromName(QStringLiteral("app-org.kde.konsole-3f2a9c11.scope")).id, QStringLiteral("org.kde.konsole"));
        QCOMPARE(appUnitFromName(QStringLiteral("app-flatpak-org.mozilla.firefox-12345.scope")).id,
                 QStringLiteral("flatpak-org.mozilla.firefox"));
        QCOMPARE(appUnitFromName(QStringLiteral("app-firefox@3a9e.service")).id, QStringLiteral("firefox"));
        // a dash inside the id comes escaped, and is not the random part
        QCOMPARE(appUnitFromName(QStringLiteral("app-raye\\x2dsettings-77.scope")).id, QStringLiteral("raye-settings"));
        const AppUnit auto_ = appUnitFromName(QStringLiteral("app-org.kde.kalendarac@autostart.service"));
        QCOMPARE(auto_.id, QStringLiteral("org.kde.kalendarac"));
        QVERIFY(auto_.autostart);
        QVERIFY(!appUnitFromName(QStringLiteral("plasma-plasmashell.service")).isValid());
        QVERIFY(!appUnitFromName(QStringLiteral("session-2.scope")).isValid());
    }

    void cgroupFile()
    {
        const QByteArray line = "0::/user.slice/user-1000.slice/user@1000.service/app.slice/app-org.kde.dolphin-9ab0.scope\n";
        QCOMPARE(appUnitFromCgroup(line).id, QStringLiteral("org.kde.dolphin"));
        // a sub-group inside the scope still belongs to it
        const QByteArray deeper = "0::/user.slice/user-1000.slice/user@1000.service/app.slice/app-org.kde.konsole-1.scope/tab(2)\n";
        QCOMPARE(appUnitFromCgroup(deeper).id, QStringLiteral("org.kde.konsole"));
        QVERIFY(!appUnitFromCgroup("0::/system.slice/cron.service\n").isValid());
        QVERIFY(!appUnitFromCgroup("").isValid());
    }

    void desktopEntries()
    {
        const QByteArray text = "[Desktop Entry]\nName=Files\nName[fr]=Fichiers\nName[fr_CA]=Fichiers (CA)\n"
                                "Icon=system-file-manager\nExec=dolphin %u\nHidden=true\n"
                                "X-GNOME-Autostart-enabled=false\nOnlyShowIn=KDE;GNOME;\n"
                                "[Desktop Action new]\nName=New window\n";
        DesktopEntry e = parseDesktopEntry(text, QStringLiteral("fr_FR"));
        QVERIFY(e.valid);
        QCOMPARE(e.name, QStringLiteral("Fichiers"));
        QCOMPARE(e.icon, QStringLiteral("system-file-manager"));
        QVERIFY(e.hidden);
        QVERIFY(!e.autostartEnabled);
        QCOMPARE(e.onlyShowIn, (QStringList{QStringLiteral("KDE"), QStringLiteral("GNOME")}));
        QCOMPARE(parseDesktopEntry(text, QStringLiteral("fr_CA")).name, QStringLiteral("Fichiers (CA)"));
        // the name of an action must not be taken for the app's
        QCOMPARE(parseDesktopEntry(text, QStringLiteral("en_US")).name, QStringLiteral("Files"));
        QVERIFY(!parseDesktopEntry("Name=x\n", QStringLiteral("en")).valid);
    }

    void launcherPrefix()
    {
        QCOMPARE(desktopIdCandidates(QStringLiteral("flatpak-org.mozilla.firefox")),
                 (QStringList{QStringLiteral("flatpak-org.mozilla.firefox"), QStringLiteral("org.mozilla.firefox")}));
        QCOMPARE(desktopIdCandidates(QStringLiteral("firefox")), QStringList{QStringLiteral("firefox")});
    }

    void appsThenBackground()
    {
        const QList<ProcEntry> entries = {
            proc(10, QStringLiteral("konsole"), QStringLiteral("org.kde.konsole"), 1, 100),
            proc(11, QStringLiteral("bash"), QStringLiteral("org.kde.konsole"), 2, 50),
            proc(20, QStringLiteral("cron")),
            proc(30, QStringLiteral("kalendarac"), QStringLiteral("org.kde.kalendarac"), 0, 10, true),
        };
        const QList<DisplayRow> rows = buildDisplayRows(entries, {}, SortKey::Name, false, {});
        QCOMPARE(rows.size(), 5);
        QCOMPARE(rows.at(0).kind, DisplayRow::Header);
        QCOMPARE(rows.at(0).count, 1);
        QCOMPARE(rows.at(1).kind, DisplayRow::App);
        QCOMPARE(rows.at(1).count, 2);
        QCOMPARE(rows.at(1).cpu, 3.0);
        QCOMPARE(rows.at(1).memory, 150);
        QCOMPARE(rows.at(1).pid, 10);
        QCOMPARE(rows.at(1).pids, (QList<int>{10, 11}));
        QCOMPARE(rows.at(2).kind, DisplayRow::Header);
        QCOMPARE(rows.at(2).section, 1);
        QCOMPARE(rows.at(2).count, 2);
        // sorted by name in the background section: cron, then kalendarac
        QCOMPARE(rows.at(3).name, QStringLiteral("cron"));
        QCOMPARE(rows.at(4).kind, DisplayRow::App);
    }

    void expandedGroupShowsItsProcesses()
    {
        const QList<ProcEntry> entries = {
            proc(10, QStringLiteral("konsole"), QStringLiteral("org.kde.konsole"), 1),
            proc(11, QStringLiteral("bash"), QStringLiteral("org.kde.konsole"), 5),
        };
        const QList<DisplayRow> rows = buildDisplayRows(entries, {QStringLiteral("app:org.kde.konsole")}, SortKey::Cpu, true, {});
        QCOMPARE(rows.size(), 4);
        QVERIFY(rows.at(1).expanded);
        QCOMPARE(rows.at(2).kind, DisplayRow::Child);
        QCOMPARE(rows.at(2).name, QStringLiteral("bash"));
        QCOMPARE(rows.at(3).name, QStringLiteral("konsole"));
    }

    void sortByCpuDescending()
    {
        const QList<ProcEntry> entries = {proc(1, QStringLiteral("a"), {}, 1), proc(2, QStringLiteral("b"), {}, 9),
                                          proc(3, QStringLiteral("c"), {}, 4)};
        const QList<DisplayRow> rows = buildDisplayRows(entries, {}, SortKey::Cpu, true, {});
        QCOMPARE(rows.at(1).pid, 2);
        QCOMPARE(rows.at(2).pid, 3);
        QCOMPARE(rows.at(3).pid, 1);
    }

    void filterByNameOrNumber()
    {
        const QList<ProcEntry> entries = {
            proc(10, QStringLiteral("konsole"), QStringLiteral("org.kde.konsole")),
            proc(11, QStringLiteral("bash"), QStringLiteral("org.kde.konsole")),
            proc(20, QStringLiteral("cron")),
            proc(4242, QStringLiteral("other")),
        };
        // a process name inside a group: the group stays, with that row
        QList<DisplayRow> rows = buildDisplayRows(entries, {}, SortKey::Name, false, QStringLiteral("BASH"));
        QCOMPARE(rows.size(), 3);
        QCOMPARE(rows.at(1).kind, DisplayRow::App);
        QVERIFY(rows.at(1).expanded);
        QCOMPARE(rows.at(2).name, QStringLiteral("bash"));
        rows = buildDisplayRows(entries, {}, SortKey::Name, false, QStringLiteral("4242"));
        QCOMPARE(rows.size(), 2);
        QCOMPARE(rows.at(1).pid, 4242);
        // a section with nothing left loses its header too
        QCOMPARE(rows.at(0).section, 1);
    }

    void kernelThreadsAreLeftOut()
    {
        ProcEntry k = proc(2, QStringLiteral("kthreadd"));
        k.kernelThread = true;
        const QList<DisplayRow> rows = buildDisplayRows({k, proc(3, QStringLiteral("cron"))}, {}, SortKey::Name, false, {});
        QCOMPARE(rows.size(), 2);
        QCOMPARE(rows.at(1).name, QStringLiteral("cron"));
    }

    void diskSumsOnlyWhatIsKnown()
    {
        ProcEntry a = proc(1, QStringLiteral("a"), QStringLiteral("x.app"));
        ProcEntry b = proc(2, QStringLiteral("b"), QStringLiteral("x.app"));
        a.disk = 1000;
        b.disk = -1;
        const QList<DisplayRow> rows = buildDisplayRows({a, b}, {}, SortKey::Name, false, {});
        QCOMPARE(rows.at(1).disk, 1000.0);
        ProcEntry c = proc(3, QStringLiteral("c"));
        QCOMPARE(buildDisplayRows({c}, {}, SortKey::Name, false, {}).at(1).disk, -1.0);
    }

    void treeOfDescendants()
    {
        const QHash<int, int> parent = {{2, 1}, {3, 2}, {4, 2}, {5, 9}, {6, 4}};
        QList<int> d = descendants(1, parent);
        std::sort(d.begin(), d.end());
        QCOMPARE(d, (QList<int>{2, 3, 4, 6}));
        QVERIFY(descendants(5, parent).isEmpty());
    }

    void priorities()
    {
        QCOMPARE(priorityForNice(19), Priority::Low);
        QCOMPARE(priorityForNice(10), Priority::BelowNormal);
        QCOMPARE(priorityForNice(0), Priority::Normal);
        QCOMPARE(priorityForNice(-5), Priority::AboveNormal);
        QCOMPARE(priorityForNice(-20), Priority::High);
        for (Priority p : {Priority::Low, Priority::BelowNormal, Priority::Normal, Priority::AboveNormal, Priority::High}) {
            QCOMPARE(priorityForNice(niceForPriority(p)), p);
        }
    }
};

QTEST_GUILESS_MAIN(TestAppGroups)
#include "test_appgroups.moc"
