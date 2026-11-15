// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtask.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace RunTask
{

namespace
{

struct Alias {
    const char *names;
    const char *desktopIds;
};

const Alias s_aliases[] = {
    {"cmd powershell pwsh wt terminal", "org.raye.terminal org.kde.konsole"},
    {"notepad wordpad write", "org.raye.notepad org.kde.kwrite org.kde.kate"},
    {"calc", "org.raye.calculator org.kde.kcalc"},
    {"control", "org.raye.controlpanel org.raye.settings"},
    {"regedit regedt32", "org.raye.regedit"},
    {"explorer", "org.kde.dolphin"},
    {"taskmgr resmon perfmon", "org.raye.taskmanager org.kde.plasma-systemmonitor"},
    {"mspaint", "org.kde.kolourpaint"},
    {"snippingtool", "org.raye.snip org.kde.spectacle"},
    {"charmap", "org.kde.kcharselect"},
    {"msedge iexplore", "firefox org.kde.falkon"},
};

// Exec= and Terminal= of the [Desktop Entry] group; QSettings would split
// an Exec line on its commas
bool opensWindow(const QString &desktopFile, const QString &programName)
{
    QFile f(desktopFile);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    bool inEntry = false;
    QString exec;
    bool terminal = false;
    while (!f.atEnd()) {
        const QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.startsWith(QLatin1Char('[')))
            inEntry = line == QLatin1String("[Desktop Entry]");
        else if (inEntry && line.startsWith(QLatin1String("Exec=")))
            exec = line.mid(5);
        else if (inEntry && line.startsWith(QLatin1String("Terminal=")))
            terminal = line.mid(9).trimmed() == QLatin1String("true");
    }
    const QStringList words = QProcess::splitCommand(exec);
    return !terminal && !words.isEmpty() && QFileInfo(words.first()).fileName() == programName;
}

const char s_systemPath[] = "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin";

}

QStringList windowsAlias(const QString &name)
{
    const QString key = name.toLower();
    for (const Alias &a : s_aliases) {
        if (QString::fromLatin1(a.names).split(QLatin1Char(' ')).contains(key))
            return QString::fromLatin1(a.desktopIds).split(QLatin1Char(' '));
    }
    return {};
}

Plan plan(const QString &command, bool asRoot, const Environment &env)
{
    Plan p;
    QStringList args = QProcess::splitCommand(command.trimmed());
    if (args.isEmpty())
        return p;
    const QString first = args.takeFirst();

    const QString app = env.installedApp(windowsAlias(first));
    QString program = env.findExecutable(first, asRoot);
    // "regedit" is Raye's Registry Editor, not the Windows layer's launcher
    if (!app.isEmpty() && (program.isEmpty() || env.isWindowsShim(program))) {
        p.kind = asRoot ? NeedsWindow : App;
        p.desktopId = app;
        return p;
    }
    if (program.isEmpty())
        return p;
    if (!asRoot) {
        p.kind = Program;
        p.program = program;
        p.args = args;
        return p;
    }
    if (env.hasWindow(program)) {
        p.kind = NeedsWindow;
        return p;
    }
    p.kind = AsRoot;
    p.program = QStringLiteral("pkexec");
    p.args = QStringList{program} + args;
    return p;
}

Environment systemEnvironment()
{
    Environment env;
    env.findExecutable = [](const QString &name, bool systemOnly) {
        if (name.contains(QLatin1Char('/'))) {
            const QFileInfo f(name);
            return f.isFile() && f.isExecutable() ? f.absoluteFilePath() : QString();
        }
        if (!systemOnly)
            return QStandardPaths::findExecutable(name);
        return QStandardPaths::findExecutable(name, QString::fromLatin1(s_systemPath).split(QLatin1Char(':')));
    };
    env.isWindowsShim = [](const QString &program) {
        const QString target = QFileInfo(program).canonicalFilePath();
        return target.contains(QLatin1String("/wine-")) || target.contains(QLatin1String("/wine/"));
    };
    env.installedApp = [](const QStringList &ids) {
        for (const QString &id : ids) {
            if (!QStandardPaths::locate(QStandardPaths::ApplicationsLocation, id + QStringLiteral(".desktop")).isEmpty())
                return id;
        }
        return QString();
    };
    // a program some menu entry opens, without a terminal, is one with a window
    env.hasWindow = [](const QString &program) {
        const QString name = QFileInfo(program).fileName();
        const auto dirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
        for (const QString &dir : dirs) {
            QDirIterator it(dir, {QStringLiteral("*.desktop")}, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                if (opensWindow(it.next(), name))
                    return true;
            }
        }
        return false;
    };
    return env;
}

QString start(const Plan &plan, QObject *owner, const std::function<void(int, const QString &)> &done)
{
    switch (plan.kind) {
    case NotFound:
        return QStringLiteral("notfound");
    case NeedsWindow:
        return QStringLiteral("window");
    case App: {
        if (QProcess::startDetached(QStringLiteral("kstart"), {QStringLiteral("--application"), plan.desktopId}))
            return QStringLiteral("started");
        const QString file = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, plan.desktopId + QStringLiteral(".desktop"));
        return !file.isEmpty() && QProcess::startDetached(QStringLiteral("gio"), {QStringLiteral("launch"), file})
            ? QStringLiteral("started")
            : QStringLiteral("notfound");
    }
    case Program:
        return QProcess::startDetached(plan.program, plan.args, QDir::homePath()) ? QStringLiteral("started")
                                                                                 : QStringLiteral("notfound");
    case AsRoot:
        break;
    }
    auto *proc = new QProcess(owner);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    proc->setWorkingDirectory(QDir::homePath());
    QObject::connect(proc, &QProcess::finished, proc, [proc, done](int code, QProcess::ExitStatus status) {
        // the end is what tells what went wrong
        const QString out = QString::fromLocal8Bit(proc->readAll()).right(4000).trimmed();
        proc->deleteLater();
        if (done)
            done(status == QProcess::NormalExit ? code : -1, out);
    });
    QObject::connect(proc, &QProcess::errorOccurred, proc, [proc, done](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        proc->deleteLater();
        if (done)
            done(127, QString());
    });
    proc->start(plan.program, plan.args);
    return QStringLiteral("waiting");
}

}
