// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

// A settings file kept line by line, so that changing one value leaves
// every other byte of the file as it was: comments, blank lines, order,
// the spelling of the other lines.
namespace Reg
{

enum class Format {
    KConfig, // ~/.config/*rc, /etc/xdg: KConfig escapes, nested [a][b] groups, [$i] locks
    Ini, // systemd, sddm, NetworkManager, gtk settings.ini: raw values
    Shell, // /etc/default/grub, /etc/environment: KEY=value, quoted
    Plain, // /etc/hostname: the whole file is one value
};

struct Entry {
    QString key;
    QString text;
    bool locked = false;
};

class SettingsFile
{
public:
    static SettingsFile parse(const QByteArray &data, Format format);

    Format format() const { return m_format; }
    // [$i] on the first line: the administrator locked the whole file
    bool locked() const { return m_locked; }

    // every group, in file order; the top of the file is the empty list
    QList<QStringList> groups() const;
    bool groupLocked(const QStringList &group) const;
    QList<Entry> entries(const QStringList &group) const;
    std::optional<Entry> entry(const QStringList &group, const QString &key) const;

    // false with *error set when the value cannot be written here
    bool set(const QStringList &group, const QString &key, const QString &text, QString *error = nullptr);
    bool remove(const QStringList &group, const QString &key, QString *error = nullptr);

    QByteArray toBytes() const;

    static QString unescapeKConfig(const QString &raw);
    static QString escapeKConfig(const QString &text);
    static QString unquoteShell(const QString &raw);
    static QString quoteShell(const QString &text);

private:
    struct Line {
        enum Kind { Other, Group, Entry };
        QByteArray raw;
        Kind kind = Other;
        QStringList group; // the group this line belongs to
        QString key; // with its [locale], without its [$...] options
        QString text; // the value, unescaped
        bool locked = false;
        QString prefix; // shell: "export " in front of the name
    };

    QString encode(const Line &line) const;
    bool writable(const QStringList &group, const QString &key, QString *error) const;
    int lastLineOf(const QStringList &group) const;

    Format m_format = Format::KConfig;
    bool m_locked = false;
    QList<Line> m_lines;
    QList<QStringList> m_lockedGroups;
    bool m_endsWithNewline = true;
};

}
