// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtask.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class TestRunTask : public QObject
{
    Q_OBJECT

    // a PC with Raye's apps, the Windows layer's launchers and a few programs
    RunTask::Environment pc()
    {
        RunTask::Environment env;
        env.findExecutable = [](const QString &name, bool systemOnly) -> QString {
            if (name == "regedit" || name == "notepad" || name == "mspaint")
                return "/usr/bin/" + name; // the Windows layer's launchers
            if (name == "kate" || name == "apt" || name == "raye-files")
                return "/usr/bin/" + name;
            if (name == "mytool" && !systemOnly)
                return "/home/u/.local/bin/mytool";
            return {};
        };
        env.isWindowsShim = [](const QString &path) { return path != "/usr/bin/kate" && path != "/usr/bin/apt"; };
        env.installedApp = [](const QStringList &ids) -> QString {
            for (const QString &id : ids) {
                if (id == "org.raye.regedit" || id == "org.raye.taskmanager" || id == "org.raye.controlpanel"
                    || id == "org.kde.konsole" || id == "org.kde.kate")
                    return id;
            }
            return {};
        };
        env.hasWindow = [](const QString &path) { return path == "/usr/bin/kate" || path == "/usr/bin/raye-files"; };
        return env;
    }

    // a folder of stand-ins put alone in PATH, so start() finds nothing else
    QTemporaryDir m_bin;
    QByteArray m_path;

    void standIn(const QString &name, const QByteArray &script)
    {
        QFile f(m_bin.filePath(name));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("#!/bin/sh\n" + script);
        f.close();
        QVERIFY(f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    }

    QByteArray waitFor(const QString &file)
    {
        QFile f(m_bin.filePath(file));
        for (int i = 0; i < 100 && !f.exists(); ++i)
            QTest::qWait(50);
        if (!f.open(QIODevice::ReadOnly))
            return {};
        return f.readAll().trimmed();
    }

private Q_SLOTS:
    void init()
    {
        QDir(m_bin.path()).removeRecursively();
        QDir().mkpath(m_bin.path());
        m_path = qgetenv("PATH");
        qputenv("PATH", QFile::encodeName(m_bin.path()));
    }

    void cleanup()
    {
        qputenv("PATH", m_path);
    }

    void windowsNamesOpenRayeApps()
    {
        const auto env = pc();
        QCOMPARE(RunTask::plan("regedit", false, env).desktopId, QStringLiteral("org.raye.regedit"));
        QCOMPARE(RunTask::plan("TaskMgr", false, env).desktopId, QStringLiteral("org.raye.taskmanager"));
        QCOMPARE(RunTask::plan("control", false, env).desktopId, QStringLiteral("org.raye.controlpanel"));
        QCOMPARE(RunTask::plan("cmd", false, env).desktopId, QStringLiteral("org.kde.konsole"));
        QCOMPARE(RunTask::plan("notepad", false, env).desktopId, QStringLiteral("org.kde.kate"));
        // no Raye app for it: the Windows layer's own one is fine
        QCOMPARE(RunTask::plan("mspaint", false, env).kind, RunTask::Program);
    }

    void plainPrograms()
    {
        const auto env = pc();
        const auto p = RunTask::plan("kate /tmp/a.txt", false, env);
        QCOMPARE(p.kind, RunTask::Program);
        QCOMPARE(p.program, QStringLiteral("/usr/bin/kate"));
        QCOMPARE(p.args, QStringList{"/tmp/a.txt"});
        QCOMPARE(RunTask::plan("nothere", false, env).kind, RunTask::NotFound);
        QCOMPARE(RunTask::plan("   ", false, env).kind, RunTask::NotFound);
    }

    // pkexec drops the display: a program with a window would never show
    void rootRefusesWindows()
    {
        const auto env = pc();
        QCOMPARE(RunTask::plan("kate", true, env).kind, RunTask::NeedsWindow);
        QCOMPARE(RunTask::plan("raye-files", true, env).kind, RunTask::NeedsWindow);
        QCOMPARE(RunTask::plan("regedit", true, env).kind, RunTask::NeedsWindow);
        const auto apt = RunTask::plan("apt update", true, env);
        QCOMPARE(apt.kind, RunTask::AsRoot);
        QCOMPARE(apt.program, QStringLiteral("pkexec"));
        QCOMPARE(apt.args, (QStringList{"/usr/bin/apt", "update"}));
    }

    // root runs what the system has, never a program the user's own PATH puts first
    void rootUsesTheSystemPath()
    {
        const auto env = pc();
        QCOMPARE(RunTask::plan("mytool", false, env).kind, RunTask::Program);
        QCOMPARE(RunTask::plan("mytool", true, env).kind, RunTask::NotFound);
    }

    // pkexec is waited for: its code and what it printed come back
    void rootRunIsWaitedFor()
    {
        standIn("pkexec", "echo \"ran $*\"\necho oops >&2\n/bin/sleep 0.3\nexit 3\n");
        RunTask::Plan p;
        p.kind = RunTask::AsRoot;
        p.program = "pkexec";
        p.args = {"/usr/bin/apt", "update"};
        int code = -100;
        QString output;
        QCOMPARE(RunTask::start(p, this, [&](int c, const QString &o) { code = c; output = o; }),
                 QStringLiteral("waiting"));
        QCOMPARE(code, -100);
        QTRY_COMPARE_WITH_TIMEOUT(code, 3, 5000);
        QVERIFY2(output.contains("ran /usr/bin/apt update"), qPrintable(output));
        QVERIFY2(output.contains("oops"), qPrintable(output));
    }

    void rootRunWithoutPkexec()
    {
        RunTask::Plan p;
        p.kind = RunTask::AsRoot;
        p.program = "pkexec";
        p.args = {"/usr/bin/apt"};
        int code = -100;
        QCOMPARE(RunTask::start(p, this, [&](int c, const QString &) { code = c; }), QStringLiteral("waiting"));
        QTRY_COMPARE_WITH_TIMEOUT(code, 127, 5000);
    }

    void appsOpenThroughKstart()
    {
        standIn("kstart", "echo \"$*\" > \"$0.args\"\n");
        RunTask::Plan p;
        p.kind = RunTask::App;
        p.desktopId = "org.raye.regedit";
        QCOMPARE(RunTask::start(p, this, {}), QStringLiteral("started"));
        QCOMPARE(waitFor("kstart.args"), QByteArray("--application org.raye.regedit"));
    }

    void programsStartDetached()
    {
        standIn("tool", "echo \"$*\" > \"$0.args\"\n");
        RunTask::Plan p;
        p.kind = RunTask::Program;
        p.program = m_bin.filePath("tool");
        p.args = {"a b", "c"};
        QCOMPARE(RunTask::start(p, this, {}), QStringLiteral("started"));
        QCOMPARE(waitFor("tool.args"), QByteArray("a b c"));
        p.program = m_bin.filePath("gone");
        QCOMPARE(RunTask::start(p, this, {}), QStringLiteral("notfound"));
        QCOMPARE(RunTask::start(RunTask::Plan{RunTask::NeedsWindow, {}, {}, {}}, this, {}), QStringLiteral("window"));
    }
};

QTEST_GUILESS_MAIN(TestRunTask)
#include "test_runtask.moc"
