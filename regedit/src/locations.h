// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "settingsfile.h"

#include <QString>
#include <QStringList>

// Which files the editor shows, and in which format it reads each one.
// systemWritable() is also the whole allow-list of the admin helper.
namespace Reg
{

// the chosen /etc files (not /etc/xdg): shown under HKEY_LOCAL_MACHINE\System
QStringList systemFiles(const QString &root = QStringLiteral("/"));
// /etc/os-release and the like: shown, never written
QStringList informationFiles(const QString &root = QStringLiteral("/"));

// may the helper write this path? Absolute, normalised, on the list.
// Does not look at the disk: the helper checks links and owners itself.
bool systemWritable(const QString &path);

// the package that ships this /etc file as a conffile, or empty.
// root: where / is, for the tests
QString packageOf(const QString &path, const QString &root = QString());

// .dpkg-dist, .dpkg-old, .ucf-dist and the like: left by updates, never settings
bool isPackageLeftover(const QString &name);

Format formatFor(const QString &path);

// does this file read as a settings file? (first real line is [group] or key=value)
bool looksLikeSettings(const QByteArray &head);

// the user files under ~/.config worth showing, relative to it
QStringList userFiles(const QString &configDir, int depth = 3);

}
