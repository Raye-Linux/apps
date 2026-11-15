// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appgroups.h"

#include <QMap>

#include <algorithm>
#include <climits>

namespace Raye::TaskManager
{

namespace
{
bool isRandomPart(const QString &text)
{
    if (text.isEmpty()) {
        return false;
    }
    for (const QChar c : text) {
        if (!c.isDigit() && !(c >= QLatin1Char('a') && c <= QLatin1Char('f')) && !(c >= QLatin1Char('A') && c <= QLatin1Char('F'))) {
            return false;
        }
    }
    return true;
}

QStringList listValue(const QString &value)
{
    QStringList out = value.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (QString &s : out) {
        s = s.trimmed();
    }
    return out;
}

bool lessByKey(SortKey key, const QString &nameA, double cpuA, qlonglong memA, double diskA, int pidA, const QString &nameB,
               double cpuB, qlonglong memB, double diskB, int pidB)
{
    switch (key) {
    case SortKey::Cpu:
        if (cpuA != cpuB) {
            return cpuA < cpuB;
        }
        break;
    case SortKey::Memory:
        if (memA != memB) {
            return memA < memB;
        }
        break;
    case SortKey::Disk:
        if (diskA != diskB) {
            return diskA < diskB;
        }
        break;
    case SortKey::Pid:
        return pidA < pidB;
    case SortKey::Name:
        break;
    }
    // not localeAwareCompare: this runs thousands of times a second, and
    // process names are plain enough for a case-blind compare
    const int byName = nameA.compare(nameB, Qt::CaseInsensitive);
    if (byName != 0) {
        return byName < 0;
    }
    return pidA < pidB;
}

DisplayRow rowOf(const ProcEntry &e, DisplayRow::Kind kind, int section)
{
    DisplayRow r;
    r.kind = kind;
    r.section = section;
    r.key = QStringLiteral("pid:%1").arg(e.pid);
    r.name = e.name;
    r.pid = e.pid;
    r.count = 1;
    r.cpu = e.cpu;
    r.memory = e.memory;
    r.disk = e.disk;
    r.efficiency = e.efficiency;
    r.state = e.state;
    r.pids = {e.pid};
    return r;
}

bool matches(const ProcEntry &e, const QString &filter)
{
    return e.name.contains(filter, Qt::CaseInsensitive) || QString::number(e.pid).contains(filter);
}
}

QString unescapeUnitName(const QString &text)
{
    QString out;
    out.reserve(text.size());
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i) == QLatin1Char('\\') && i + 3 < text.size() && text.at(i + 1) == QLatin1Char('x')) {
            bool ok = false;
            const int code = text.mid(i + 2, 2).toInt(&ok, 16);
            if (ok) {
                out.append(QChar(code));
                i += 3;
                continue;
            }
        }
        out.append(text.at(i));
    }
    return out;
}

AppUnit appUnitFromName(const QString &unit)
{
    AppUnit out;
    if (!unit.startsWith(QLatin1String("app-"))) {
        return out;
    }
    QString body;
    if (unit.endsWith(QLatin1String(".scope"))) {
        body = unit.mid(4, unit.size() - 4 - 6);
    } else if (unit.endsWith(QLatin1String(".service"))) {
        body = unit.mid(4, unit.size() - 4 - 8);
    } else {
        return out;
    }
    const int at = body.indexOf(QLatin1Char('@'));
    if (at >= 0) {
        out.autostart = body.mid(at + 1) == QLatin1String("autostart");
        body = body.left(at);
    } else {
        // "<id>-<random>": the id's own dashes are escaped, so the last
        // plain dash is the one before the random part
        const int dash = body.lastIndexOf(QLatin1Char('-'));
        if (dash > 0 && isRandomPart(body.mid(dash + 1))) {
            body = body.left(dash);
        }
    }
    out.id = unescapeUnitName(body);
    return out;
}

AppUnit appUnitFromCgroup(const QByteArray &cgroupFile)
{
    for (const QByteArray &line : cgroupFile.split('\n')) {
        if (!line.startsWith("0::")) {
            continue;
        }
        const QStringList parts = QString::fromUtf8(line.mid(3)).trimmed().split(QLatin1Char('/'), Qt::SkipEmptyParts);
        // the innermost app unit; groups below it belong to it
        for (int i = parts.size() - 1; i >= 0; --i) {
            const AppUnit unit = appUnitFromName(parts.at(i));
            if (unit.isValid()) {
                return unit;
            }
        }
    }
    return {};
}

DesktopEntry parseDesktopEntry(const QByteArray &text, const QString &lang)
{
    DesktopEntry e;
    const QString country = lang.section(QLatin1Char('.'), 0, 0).section(QLatin1Char('@'), 0, 0);
    const QString language = country.section(QLatin1Char('_'), 0, 0);
    int nameRank = -1;
    bool inGroup = false;
    for (const QByteArray &raw : text.split('\n')) {
        const QString line = QString::fromUtf8(raw).trimmed();
        if (line.startsWith(QLatin1Char('['))) {
            inGroup = line == QLatin1String("[Desktop Entry]");
            e.valid = e.valid || inGroup;
            continue;
        }
        if (!inGroup || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq < 0) {
            continue;
        }
        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();
        if (key.startsWith(QLatin1String("Name"))) {
            int rank = -1;
            if (key == QLatin1String("Name")) {
                rank = 0;
            } else if (key == QStringLiteral("Name[%1]").arg(language)) {
                rank = 1;
            } else if (country != language && key == QStringLiteral("Name[%1]").arg(country)) {
                rank = 2;
            }
            if (rank > nameRank) {
                nameRank = rank;
                e.name = value;
            }
        } else if (key == QLatin1String("Icon")) {
            e.icon = value;
        } else if (key == QLatin1String("Exec")) {
            e.exec = value;
        } else if (key == QLatin1String("Comment")) {
            e.comment = value;
        } else if (key == QLatin1String("Hidden")) {
            e.hidden = value == QLatin1String("true");
        } else if (key == QLatin1String("NoDisplay")) {
            e.noDisplay = value == QLatin1String("true");
        } else if (key == QLatin1String("X-GNOME-Autostart-enabled")) {
            e.autostartEnabled = value != QLatin1String("false");
        } else if (key == QLatin1String("OnlyShowIn")) {
            e.onlyShowIn = listValue(value);
        } else if (key == QLatin1String("NotShowIn")) {
            e.notShowIn = listValue(value);
        }
    }
    return e;
}

QStringList desktopIdCandidates(const QString &id)
{
    QStringList out{id};
    for (const QString &launcher : {QStringLiteral("flatpak-"), QStringLiteral("gnome-"), QStringLiteral("kde-")}) {
        if (id.startsWith(launcher) && id.size() > launcher.size()) {
            out.append(id.mid(launcher.size()));
        }
    }
    return out;
}

QList<DisplayRow> buildDisplayRows(const QList<ProcEntry> &entries, const QSet<QString> &expanded, SortKey key, bool descending,
                                   const QString &filter)
{
    const QString text = filter.trimmed();
    // groups by app id, in the order met
    QMap<QString, QList<ProcEntry>> groups;
    QList<ProcEntry> loose;
    for (const ProcEntry &e : entries) {
        if (e.kernelThread) {
            continue;
        }
        if (e.appId.isEmpty()) {
            if (text.isEmpty() || matches(e, text)) {
                loose.append(e);
            }
        } else {
            groups[e.appId].append(e);
        }
    }

    QList<DisplayRow> sections[2];
    QHash<QString, QList<DisplayRow>> children;

    const auto less = [key, descending](const DisplayRow &a, const DisplayRow &b) {
        const bool l = lessByKey(key, a.name, a.cpu, a.memory, a.disk, a.pid, b.name, b.cpu, b.memory, b.disk, b.pid);
        const bool g = lessByKey(key, b.name, b.cpu, b.memory, b.disk, b.pid, a.name, a.cpu, a.memory, a.disk, a.pid);
        return descending ? g : l;
    };

    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it) {
        const QList<ProcEntry> &members = it.value();
        const ProcEntry &first = members.first();
        const QString name = first.appName.isEmpty() ? first.appId : first.appName;
        const bool nameMatches = text.isEmpty() || name.contains(text, Qt::CaseInsensitive);
        const int section = first.autostart ? 1 : 0;

        DisplayRow group;
        group.kind = DisplayRow::App;
        group.section = section;
        group.key = QStringLiteral("app:") + it.key();
        group.name = name;
        group.icon = first.appIcon;
        group.pid = INT_MAX;
        group.state = first.state;
        QList<DisplayRow> kids;
        bool anyKnownDisk = false;
        double disk = 0;
        for (const ProcEntry &e : members) {
            group.count++;
            group.cpu += e.cpu;
            group.memory += e.memory;
            group.pid = qMin(group.pid, e.pid);
            group.pids.append(e.pid);
            group.efficiency = group.efficiency || e.efficiency;
            if (e.disk >= 0) {
                anyKnownDisk = true;
                disk += e.disk;
            }
            if (nameMatches || matches(e, text)) {
                kids.append(rowOf(e, DisplayRow::Child, section));
            }
        }
        if (kids.isEmpty()) {
            continue;
        }
        group.disk = anyKnownDisk ? disk : -1.0;
        std::sort(group.pids.begin(), group.pids.end());
        // a group found through one of its processes opens, so the match shows
        group.expanded = expanded.contains(group.key) || !nameMatches;
        std::sort(kids.begin(), kids.end(), less);
        children.insert(group.key, kids);
        sections[section].append(group);
    }
    for (const ProcEntry &e : loose) {
        sections[1].append(rowOf(e, DisplayRow::Process, 1));
    }

    QList<DisplayRow> out;
    for (int s = 0; s < 2; ++s) {
        if (sections[s].isEmpty()) {
            continue;
        }
        std::sort(sections[s].begin(), sections[s].end(), less);
        DisplayRow header;
        header.kind = DisplayRow::Header;
        header.section = s;
        header.key = QStringLiteral("section:%1").arg(s);
        header.count = int(sections[s].size());
        out.append(header);
        for (const DisplayRow &row : std::as_const(sections[s])) {
            out.append(row);
            if (row.kind == DisplayRow::App && row.expanded) {
                out.append(children.value(row.key));
            }
        }
    }
    return out;
}

QList<int> descendants(int pid, const QHash<int, int> &parentOf)
{
    QHash<int, QList<int>> childrenOf;
    for (auto it = parentOf.constBegin(); it != parentOf.constEnd(); ++it) {
        childrenOf[it.value()].append(it.key());
    }
    QList<int> out;
    QList<int> todo{pid};
    QSet<int> seen{pid};
    while (!todo.isEmpty()) {
        const int current = todo.takeFirst();
        for (int child : childrenOf.value(current)) {
            if (seen.contains(child)) {
                continue;
            }
            seen.insert(child);
            out.append(child);
            todo.append(child);
        }
    }
    return out;
}

Priority priorityForNice(int nice)
{
    if (nice >= 15) {
        return Priority::Low;
    }
    if (nice >= 5) {
        return Priority::BelowNormal;
    }
    if (nice > -5) {
        return Priority::Normal;
    }
    if (nice > -15) {
        return Priority::AboveNormal;
    }
    return Priority::High;
}

int niceForPriority(Priority priority)
{
    switch (priority) {
    case Priority::Low:
        return 19;
    case Priority::BelowNormal:
        return 10;
    case Priority::Normal:
        return 0;
    case Priority::AboveNormal:
        return -5;
    case Priority::High:
        return -15;
    }
    return 0;
}

} // namespace Raye::TaskManager
