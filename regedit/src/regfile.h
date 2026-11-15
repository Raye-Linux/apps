// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QString>

#include <optional>

// The text of File > Export, and what File > Import reads back.
//
//   Raye Registry Editor 1
//
//   [HKEY_CURRENT_USER\Config\kdeglobals\General]
//   "AccentColor"="123,60,255"
//   "Old"=-
//   "color-scheme"=gvariant:'prefer-dark'
namespace Reg
{

struct RegItem {
    QString name;
    bool remove = false;
    bool gvariant = false; // text is GVariant text, not a plain string
    QString text;
};

struct RegSection {
    QString path;
    QList<RegItem> items;
};

QString writeRegFile(const QList<RegSection> &sections);
// nullopt with *error (and *line) on the first line it cannot read
std::optional<QList<RegSection>> readRegFile(const QString &text, QString *error = nullptr, int *line = nullptr);

inline const QString RegHeader = QStringLiteral("Raye Registry Editor 1");

}
