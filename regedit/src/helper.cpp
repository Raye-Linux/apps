// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// raye-regedit-helper: the one part of the Registry Editor that runs as
// root, through pkexec (org.raye.regedit.write). It makes one change to
// one value of one file on the list in locations.cpp, and nothing else.

#include "edits.h"
#include "helperargs.h"
#include "locations.h"

#include <QCoreApplication>
#include <QFile>

#include <cstdio>
#include <unistd.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (::geteuid() != 0) {
        std::fputs("raye-regedit-helper: run through pkexec\n", stderr);
        return 2;
    }
    QFile in;
    if (!in.open(stdin, QIODevice::ReadOnly))
        return 2;
    const QByteArray input = in.read(Reg::MaxFileSize + 1);

    QString error;
    const auto change = Reg::parseHelperArguments(app.arguments().mid(1), input, &error);
    if (!change) {
        std::fprintf(stderr, "raye-regedit-helper: %s\n", qPrintable(error));
        const QStringList a = app.arguments();
        return a.size() == 5 && Reg::isCodeKey(a.at(2), a.at(4)) ? Reg::HelperCodeKey : Reg::HelperRefused;
    }
    error = Reg::refuseSystemPath(change->path);
    if (!error.isEmpty()) {
        std::fprintf(stderr, "raye-regedit-helper: %s\n", qPrintable(error));
        return Reg::packageOf(change->path).isEmpty() ? Reg::HelperRefused : Reg::HelperPackageFile;
    }
    const Reg::Outcome o = Reg::applyChange(*change, QStringLiteral("/var/backups/raye-regedit"));
    if (!o.ok) {
        std::fprintf(stderr, "raye-regedit-helper: %s\n", qPrintable(o.error));
        return Reg::HelperFailed;
    }
    // what the window needs for its undo list
    std::printf("had %d\n%s", o.hadValue ? 1 : 0, o.oldText.toUtf8().constData());
    return 0;
}
