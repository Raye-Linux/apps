// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

#include <functional>

class QObject;

// What "Run new task" does with what was typed. The names Windows users
// type open Raye's own app, as they do in Win+R (theme, shell/run): keep
// the two tables in step.
namespace RunTask
{

enum Kind {
    NotFound,
    Program, // start program with args
    App, // open the app desktopId
    AsRoot, // program is pkexec: wait for it and tell what happened
    NeedsWindow, // as root, but it opens a window, which pkexec cannot show
};

struct Plan {
    Kind kind = NotFound;
    QString program;
    QStringList args;
    QString desktopId;
};

struct Environment {
    // systemOnly: the PATH root would use, not the user's
    std::function<QString(const QString &name, bool systemOnly)> findExecutable;
    std::function<bool(const QString &path)> isWindowsShim;
    std::function<QString(const QStringList &desktopIds)> installedApp;
    std::function<bool(const QString &path)> hasWindow;
};

QStringList windowsAlias(const QString &name);
Plan plan(const QString &command, bool asRoot, const Environment &env);
Environment systemEnvironment();

// Starts what plan() chose: "started", "notfound", "window", or "waiting"
// for a root run. That one is a child of owner, and done(code, output) comes
// once pkexec ends: the program's code, 126/127 from pkexec, 127 if it could
// not start, -1 if it crashed.
QString start(const Plan &plan, QObject *owner, const std::function<void(int, const QString &)> &done);

}
