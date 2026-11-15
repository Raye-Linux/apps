// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingsfile.h"

#include <QRegularExpression>

namespace Reg
{

namespace
{

QString setError(QString *error, const QString &text)
{
    if (error)
        *error = text;
    return text;
}

// "[A][B][$i]" -> {"A", "B"}, locked
bool parseKConfigGroup(const QString &line, QStringList *group, bool *locked)
{
    group->clear();
    *locked = false;
    int i = 0;
    while (i < line.size() && line.at(i) == QLatin1Char('[')) {
        const int close = line.indexOf(QLatin1Char(']'), i + 1);
        if (close < 0)
            return false;
        const QString part = line.mid(i + 1, close - i - 1);
        if (part.startsWith(QLatin1Char('$')))
            *locked = *locked || part.contains(QLatin1Char('i'));
        else
            group->append(part);
        i = close + 1;
    }
    return i > 0 && line.mid(i).trimmed().isEmpty();
}

}

QString SettingsFile::unescapeKConfig(const QString &raw)
{
    QString out;
    out.reserve(raw.size());
    for (int i = 0; i < raw.size(); ++i) {
        const QChar c = raw.at(i);
        if (c != QLatin1Char('\\') || i + 1 >= raw.size()) {
            out.append(c);
            continue;
        }
        const QChar n = raw.at(++i);
        switch (n.unicode()) {
        case 's': out.append(QLatin1Char(' ')); break;
        case 't': out.append(QLatin1Char('\t')); break;
        case 'n': out.append(QLatin1Char('\n')); break;
        case 'r': out.append(QLatin1Char('\r')); break;
        case '\\': out.append(QLatin1Char('\\')); break;
        default:
            // \, and \; belong to lists: KConfig keeps them, so do we
            out.append(c);
            out.append(n);
        }
    }
    return out;
}

QString SettingsFile::escapeKConfig(const QString &text)
{
    QString out;
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c == QLatin1Char('\\'))
            out.append(QLatin1String("\\\\"));
        else if (c == QLatin1Char('\n'))
            out.append(QLatin1String("\\n"));
        else if (c == QLatin1Char('\t'))
            out.append(QLatin1String("\\t"));
        else if (c == QLatin1Char('\r'))
            out.append(QLatin1String("\\r"));
        else if (c == QLatin1Char(' ') && (i == 0 || i == text.size() - 1))
            out.append(QLatin1String("\\s"));
        else
            out.append(c);
    }
    return out;
}

QString SettingsFile::unquoteShell(const QString &raw)
{
    const QString v = raw.trimmed();
    if (v.size() >= 2 && v.startsWith(QLatin1Char('\'')) && v.endsWith(QLatin1Char('\'')))
        return v.mid(1, v.size() - 2);
    if (v.size() >= 2 && v.startsWith(QLatin1Char('"')) && v.endsWith(QLatin1Char('"'))) {
        QString out;
        const QString inner = v.mid(1, v.size() - 2);
        for (int i = 0; i < inner.size(); ++i) {
            if (inner.at(i) == QLatin1Char('\\') && i + 1 < inner.size()
                && QStringLiteral("\"\\$`").contains(inner.at(i + 1)))
                ++i;
            out.append(inner.at(i));
        }
        return out;
    }
    return v;
}

QString SettingsFile::quoteShell(const QString &text)
{
    static const QRegularExpression safe(QStringLiteral("^[A-Za-z0-9_./:,+@%=-]+$"));
    if (safe.match(text).hasMatch())
        return text;
    QString out = QStringLiteral("\"");
    for (const QChar c : text) {
        if (QStringLiteral("\"\\$`").contains(c))
            out.append(QLatin1Char('\\'));
        out.append(c);
    }
    out.append(QLatin1Char('"'));
    return out;
}

SettingsFile SettingsFile::parse(const QByteArray &data, Format format)
{
    SettingsFile f;
    f.m_format = format;
    f.m_endsWithNewline = data.isEmpty() || data.endsWith('\n');
    QList<QByteArray> raws = data.split('\n');
    if (f.m_endsWithNewline && !raws.isEmpty())
        raws.removeLast();

    if (format == Format::Plain) {
        Line line;
        line.raw = data;
        line.kind = Line::Entry;
        line.text = QString::fromUtf8(data).trimmed();
        line.locked = true;
        f.m_lines.append(line);
        f.m_locked = true;
        return f;
    }

    static const QRegularExpression shellLine(QStringLiteral("^\\s*(export\\s+)?([A-Za-z_][A-Za-z0-9_]*)=(.*)$"));
    QStringList group;
    bool first = true;
    for (const QByteArray &raw : std::as_const(raws)) {
        Line line;
        line.raw = raw;
        QString text = QString::fromUtf8(raw);
        if (text.endsWith(QLatin1Char('\r')))
            text.chop(1);
        const QString trimmed = text.trimmed();
        line.group = group;

        if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#'))
            || (format != Format::Shell && trimmed.startsWith(QLatin1Char(';')))) {
            f.m_lines.append(line);
            continue;
        }

        if (format == Format::Shell) {
            const auto m = shellLine.match(text);
            if (m.hasMatch()) {
                line.kind = Line::Entry;
                line.prefix = m.captured(1);
                line.key = m.captured(2);
                line.text = unquoteShell(m.captured(3));
            }
            f.m_lines.append(line);
            continue;
        }

        if (trimmed.startsWith(QLatin1Char('['))) {
            QStringList g;
            bool locked = false;
            if (format == Format::KConfig) {
                if (!parseKConfigGroup(trimmed, &g, &locked)) {
                    f.m_lines.append(line);
                    continue;
                }
            } else {
                if (!trimmed.endsWith(QLatin1Char(']'))) {
                    f.m_lines.append(line);
                    continue;
                }
                g = QStringList{trimmed.mid(1, trimmed.size() - 2)};
            }
            if (g.isEmpty()) {
                // "[$i]" alone: on the first line it locks the whole file
                if (first && locked)
                    f.m_locked = true;
                f.m_lines.append(line);
                first = false;
                continue;
            }
            group = g;
            line.kind = Line::Group;
            line.group = g;
            if (locked && !f.m_lockedGroups.contains(g))
                f.m_lockedGroups.append(g);
            f.m_lines.append(line);
            first = false;
            continue;
        }
        first = false;

        const int eq = text.indexOf(QLatin1Char('='));
        if (eq <= 0) {
            f.m_lines.append(line);
            continue;
        }
        QString key = text.left(eq).trimmed();
        QString value = text.mid(eq + 1).trimmed();
        if (format == Format::KConfig) {
            // key[fr][$i]: the locale is part of the name, the options are not
            static const QRegularExpression options(QStringLiteral("\\[\\$([a-z]+)\\]$"));
            for (auto m = options.match(key); m.hasMatch(); m = options.match(key)) {
                if (m.captured(1).contains(QLatin1Char('i')))
                    line.locked = true;
                key.chop(m.capturedLength());
            }
            value = unescapeKConfig(value);
        }
        line.kind = Line::Entry;
        line.key = key;
        line.text = value;
        f.m_lines.append(line);
    }
    return f;
}

QList<QStringList> SettingsFile::groups() const
{
    QList<QStringList> out;
    out.append(QStringList());
    for (const Line &l : m_lines) {
        if (l.kind == Line::Group && !out.contains(l.group))
            out.append(l.group);
    }
    return out;
}

bool SettingsFile::groupLocked(const QStringList &group) const
{
    return m_locked || m_lockedGroups.contains(group);
}

QList<Entry> SettingsFile::entries(const QStringList &group) const
{
    QList<Entry> out;
    for (const Line &l : m_lines) {
        if (l.kind != Line::Entry || l.group != group)
            continue;
        const Entry e{l.key, l.text, l.locked || groupLocked(group)};
        // a key written twice: the last one is the one read
        bool replaced = false;
        for (Entry &o : out) {
            if (o.key == e.key) {
                o = e;
                replaced = true;
            }
        }
        if (!replaced)
            out.append(e);
    }
    return out;
}

std::optional<Entry> SettingsFile::entry(const QStringList &group, const QString &key) const
{
    for (const Entry &e : entries(group)) {
        if (e.key == key)
            return e;
    }
    return std::nullopt;
}

bool SettingsFile::writable(const QStringList &group, const QString &key, QString *error) const
{
    if (m_format == Format::Plain || m_locked) {
        setError(error, QStringLiteral("locked file"));
        return false;
    }
    if (m_format == Format::Shell && !group.isEmpty()) {
        setError(error, QStringLiteral("shell files have no groups"));
        return false;
    }
    if (groupLocked(group)) {
        setError(error, QStringLiteral("locked group"));
        return false;
    }
    static const QRegularExpression shellName(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
    if (key.trimmed().isEmpty() || key != key.trimmed() || key.contains(QLatin1Char('='))
        || key.contains(QLatin1Char('\n')) || key.startsWith(QLatin1Char('[')) || key.startsWith(QLatin1Char('#'))
        || key.startsWith(QLatin1Char(';')) || (m_format == Format::Shell && !shellName.match(key).hasMatch())) {
        setError(error, QStringLiteral("bad name"));
        return false;
    }
    const auto existing = entry(group, key);
    if (existing && existing->locked) {
        setError(error, QStringLiteral("locked value"));
        return false;
    }
    return true;
}

QString SettingsFile::encode(const Line &line) const
{
    switch (m_format) {
    case Format::KConfig:
        return line.key + QLatin1Char('=') + escapeKConfig(line.text);
    case Format::Shell:
        return line.prefix + line.key + QLatin1Char('=') + quoteShell(line.text);
    default:
        return line.key + QLatin1Char('=') + line.text;
    }
}

int SettingsFile::lastLineOf(const QStringList &group) const
{
    int last = -1;
    for (int i = 0; i < m_lines.size(); ++i) {
        const Line &l = m_lines.at(i);
        if (l.group != group)
            continue;
        if (l.kind == Line::Group || l.kind == Line::Entry)
            last = i;
    }
    return last;
}

bool SettingsFile::set(const QStringList &group, const QString &key, const QString &text, QString *error)
{
    if (!writable(group, key, error))
        return false;
    if (m_format != Format::KConfig && (text.contains(QLatin1Char('\n')) || text.contains(QLatin1Char('\r')))) {
        setError(error, QStringLiteral("one line only"));
        return false;
    }
    for (int i = m_lines.size() - 1; i >= 0; --i) {
        Line &l = m_lines[i];
        if (l.kind == Line::Entry && l.group == group && l.key == key) {
            if (l.text == text)
                return true;
            l.text = text;
            // keep "key = value" or "key=value", whichever the file used
            const QString old = QString::fromUtf8(l.raw);
            const int eq = old.indexOf(QLatin1Char('='));
            QString head = old.left(eq + 1);
            QString encoded = encode(l);
            const int newEq = encoded.indexOf(QLatin1Char('='));
            QString body = encoded.mid(newEq + 1);
            if (m_format != Format::Shell && old.mid(eq + 1).startsWith(QLatin1Char(' ')))
                body.prepend(QLatin1Char(' '));
            l.raw = (head + body).toUtf8();
            if (old.endsWith(QLatin1Char('\r')))
                l.raw.append('\r');
            return true;
        }
    }

    Line line;
    line.kind = Line::Entry;
    line.group = group;
    line.key = key;
    line.text = text;
    line.raw = encode(line).toUtf8();
    const int last = lastLineOf(group);
    if (last >= 0) {
        m_lines.insert(last + 1, line);
        return true;
    }
    if (group.isEmpty()) {
        m_lines.insert(0, line);
        return true;
    }
    Line header;
    header.kind = Line::Group;
    header.group = group;
    if (m_format == Format::KConfig) {
        for (const QString &part : group)
            header.raw += '[' + part.toUtf8() + ']';
    } else {
        header.raw = '[' + group.join(QString()).toUtf8() + ']';
    }
    if (!m_lines.isEmpty()) {
        Line blank;
        blank.group = m_lines.constLast().group;
        m_lines.append(blank);
    }
    m_lines.append(header);
    m_lines.append(line);
    m_endsWithNewline = true;
    return true;
}

bool SettingsFile::remove(const QStringList &group, const QString &key, QString *error)
{
    if (!writable(group, key, error))
        return false;
    bool found = false;
    for (int i = m_lines.size() - 1; i >= 0; --i) {
        const Line &l = m_lines.at(i);
        if (l.kind == Line::Entry && l.group == group && l.key == key) {
            m_lines.removeAt(i);
            found = true;
        }
    }
    if (!found)
        setError(error, QStringLiteral("no such value"));
    return found;
}

QByteArray SettingsFile::toBytes() const
{
    if (m_format == Format::Plain)
        return m_lines.isEmpty() ? QByteArray() : m_lines.constFirst().raw;
    QByteArray out;
    for (int i = 0; i < m_lines.size(); ++i) {
        out += m_lines.at(i).raw;
        if (i + 1 < m_lines.size() || m_endsWithNewline)
            out += '\n';
    }
    return out;
}

}
