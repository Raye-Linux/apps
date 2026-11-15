// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "settingsfile.h"

#include <QString>
#include <QStringList>

// One change to one value of a settings file, done the careful way: the
// file is read again, a copy of it is kept, only the value's line
// changes, and the new file replaces the old one in one rename.
namespace Reg
{

struct Change {
    enum Op { Set, Remove };
    Op op = Set;
    QString path;
    Format format = Format::KConfig;
    QStringList group;
    QString key;
    QString text;
};

struct Outcome {
    bool ok = false;
    QString error;
    bool changed = false; // false when the file already said so
    bool hadValue = false;
    QString oldText;
    QString backup; // where the copy went
};

// backupRoot: ~/.local/state/raye-regedit/backups or /var/backups/raye-regedit
Outcome applyChange(const Change &change, const QString &backupRoot, int keepBackups = 200);

// the change that takes an applied one back
Change inverseOf(const Change &change, const Outcome &outcome);

// what types a value reads as, and whether a new text fits the old type
enum class ValueType { Text, Number, YesNo, List };
ValueType guessType(const QString &text, Format format);
bool fitsType(const QString &oldText, const QString &newText, ValueType type);

// largest settings file the editor reads or writes
constexpr qint64 MaxFileSize = 512 * 1024;

}
