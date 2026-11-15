// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "autostart.h"
#include "appgroups.h"

#include <QDir>
#include <QFile>
#include <QMap>
#include <QSaveFile>

namespace Raye::TaskManager
{

namespace
{
QByteArray readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

bool shownIn(const DesktopEntry &e, const QString &desktop)
{
    const QStringList current = desktop.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    if (!e.onlyShowIn.isEmpty()) {
        bool found = false;
        for (const QString &d : current) {
            found = found || e.onlyShowIn.contains(d);
        }
        if (!found) {
            return false;
        }
    }
    for (const QString &d : current) {
        if (e.notShowIn.contains(d)) {
            return false;
        }
    }
    return true;
}

// The [Desktop Entry] group with the two keys that switch it off taken
// out, and Hidden=true put back in when off. Every other line is kept.
QByteArray rewritten(const QByteArray &text, bool enabled)
{
    QByteArray out;
    bool inGroup = false;
    bool placed = false;
    const QList<QByteArray> lines = text.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        const QByteArray &line = lines.at(i);
        const QByteArray trimmed = line.trimmed();
        if (trimmed.startsWith('[')) {
            if (inGroup && !enabled && !placed) {
                out += "Hidden=true\n";
                placed = true;
            }
            inGroup = trimmed == "[Desktop Entry]";
        } else if (inGroup && (trimmed.startsWith("Hidden=") || trimmed.startsWith("X-GNOME-Autostart-enabled="))) {
            continue;
        }
        if (i == lines.size() - 1 && line.isEmpty()) {
            break;
        }
        out += line + '\n';
    }
    if (inGroup && !enabled && !placed) {
        out += "Hidden=true\n";
    }
    return out;
}
}

QList<AutostartEntry> listAutostart(const QStringList &systemDirs, const QString &userDir, const QString &lang,
                                    const QString &desktop)
{
    // file name -> path, lowest precedence first so the later win
    QMap<QString, QString> chosen;
    QMap<QString, bool> inSystem;
    for (auto dir = systemDirs.crbegin(); dir != systemDirs.crend(); ++dir) {
        const QStringList names = QDir(*dir).entryList({QStringLiteral("*.desktop")}, QDir::Files);
        for (const QString &name : names) {
            chosen.insert(name, QDir(*dir).filePath(name));
            inSystem.insert(name, true);
        }
    }
    const QStringList own = QDir(userDir).entryList({QStringLiteral("*.desktop")}, QDir::Files);
    for (const QString &name : own) {
        chosen.insert(name, QDir(userDir).filePath(name));
    }

    QList<AutostartEntry> out;
    for (auto it = chosen.constBegin(); it != chosen.constEnd(); ++it) {
        const DesktopEntry e = parseDesktopEntry(readFile(it.value()), lang);
        if (!e.valid || !shownIn(e, desktop)) {
            continue;
        }
        const bool personal = it.value().startsWith(userDir + QLatin1Char('/'));
        // a system file with Hidden=true is a deleted entry, not a
        // switched-off one: nothing to show
        if (e.hidden && !personal) {
            continue;
        }
        AutostartEntry a;
        a.fileName = it.key();
        a.name = e.name.isEmpty() ? it.key().chopped(8) : e.name;
        a.icon = e.icon;
        a.comment = e.comment;
        a.exec = e.exec;
        a.enabled = !e.hidden && e.autostartEnabled;
        a.systemEntry = inSystem.value(it.key());
        a.path = it.value();
        out.append(a);
    }
    return out;
}

bool setAutostartEnabled(const AutostartEntry &entry, bool enabled, const QString &userDir)
{
    const QString target = QDir(userDir).filePath(entry.fileName);
    // the person's copy if there is one, else the system file it copies
    const QByteArray source = readFile(QFile::exists(target) ? target : entry.path);
    if (source.isEmpty()) {
        return false;
    }
    if (!QDir().mkpath(userDir)) {
        return false;
    }
    QSaveFile file(target);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(rewritten(source, enabled));
    return file.commit();
}

QString escapeUnitName(const QString &text)
{
    QString out;
    const QByteArray bytes = text.toUtf8();
    for (int i = 0; i < bytes.size(); ++i) {
        const char c = bytes.at(i);
        const bool plain = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ':' || c == '_'
            || (c == '.' && i > 0);
        if (plain) {
            out.append(QLatin1Char(c));
        } else {
            out.append(QStringLiteral("\\x%1").arg(uchar(c), 2, 16, QLatin1Char('0')));
        }
    }
    return out;
}

QString autostartUnitName(const QString &fileName)
{
    QString id = fileName;
    if (id.endsWith(QLatin1String(".desktop"))) {
        id.chop(8);
    }
    return QStringLiteral("app-") + escapeUnitName(id) + QStringLiteral("@autostart.service");
}

Impact impactForCpuUsec(qlonglong usec)
{
    if (usec < 0) {
        return Impact::NotMeasured;
    }
    if (usec == 0) {
        return Impact::None;
    }
    // Windows' bands: under 300 ms low, under a second medium
    if (usec < 300000) {
        return Impact::Low;
    }
    if (usec < 1000000) {
        return Impact::Medium;
    }
    return Impact::High;
}

qlonglong parseCpuStatUsage(const QByteArray &cpuStat)
{
    for (const QByteArray &line : cpuStat.split('\n')) {
        if (line.startsWith("usage_usec ")) {
            return line.mid(11).trimmed().toLongLong();
        }
    }
    return -1;
}

} // namespace Raye::TaskManager
