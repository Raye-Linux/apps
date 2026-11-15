// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "edits.h"

#include <QByteArray>
#include <QStringList>

#include <optional>
#include <sys/types.h>

// What the admin helper accepts, kept apart from its main() so the tests
// can hold it to its word.
//
//   raye-regedit-helper set|remove <path> <group> <key>     (value on stdin)
//
// <group> is the group path joined with U+001F, or "-" for the top of the file.
namespace Reg
{

inline constexpr QChar GroupSeparator = QChar(0x1f);

QStringList helperArguments(const Change &change);

// keys of a listed file that would make a program running as root load or
// run what the value names; the helper refuses them, the window locks them
bool isCodeKey(const QString &path, const QString &key);

// exit codes of the helper, so the window can say why in its own language
enum HelperExit { HelperFailed = 1, HelperRefused = 2, HelperCodeKey = 3, HelperPackageFile = 4 };
std::optional<Change> parseHelperArguments(const QStringList &args, const QByteArray &input, QString *error);

// why the helper must not touch this file, or an empty string when it may:
// on the list, a regular file and not a link, owned by `owner`, small;
// root: where / is, for the tests
QString refuseSystemPath(const QString &path, uid_t owner = 0, const QString &root = QString());

}
