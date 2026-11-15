// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "regfile.h"

#include <QStringList>

namespace Reg
{

namespace
{

QString quoted(const QString &text)
{
    QString out = QStringLiteral("\"");
    for (const QChar c : text) {
        if (c == QLatin1Char('"') || c == QLatin1Char('\\'))
            out += QLatin1Char('\\') + c;
        else if (c == QLatin1Char('\n'))
            out += QLatin1String("\\n");
        else if (c == QLatin1Char('\t'))
            out += QLatin1String("\\t");
        else
            out += c;
    }
    return out + QLatin1Char('"');
}

// reads "..." from position *at, leaves *at after the closing quote
std::optional<QString> unquoted(const QString &line, int *at)
{
    if (*at >= line.size() || line.at(*at) != QLatin1Char('"'))
        return std::nullopt;
    QString out;
    for (int i = *at + 1; i < line.size(); ++i) {
        const QChar c = line.at(i);
        if (c == QLatin1Char('"')) {
            *at = i + 1;
            return out;
        }
        if (c == QLatin1Char('\\') && i + 1 < line.size()) {
            const QChar n = line.at(++i);
            out += n == QLatin1Char('n') ? QChar(u'\n') : n == QLatin1Char('t') ? QChar(u'\t') : n;
            continue;
        }
        out += c;
    }
    return std::nullopt;
}

}

QString writeRegFile(const QList<RegSection> &sections)
{
    QString out = RegHeader + QLatin1String("\n");
    for (const RegSection &s : sections) {
        out += QLatin1String("\n[") + s.path + QLatin1String("]\n");
        for (const RegItem &i : s.items) {
            out += quoted(i.name) + QLatin1Char('=');
            if (i.remove)
                out += QLatin1Char('-');
            else if (i.gvariant)
                out += QLatin1String("gvariant:") + i.text;
            else
                out += quoted(i.text);
            out += QLatin1Char('\n');
        }
    }
    return out;
}

std::optional<QList<RegSection>> readRegFile(const QString &text, QString *error, int *lineNo)
{
    auto fail = [&](int n, const QString &why) -> std::optional<QList<RegSection>> {
        if (error)
            *error = why;
        if (lineNo)
            *lineNo = n;
        return std::nullopt;
    };
    const QStringList lines = text.split(QLatin1Char('\n'));
    if (lines.isEmpty() || lines.constFirst().trimmed() != RegHeader)
        return fail(1, QStringLiteral("not a Raye registry file"));
    QList<RegSection> out;
    for (int n = 1; n < lines.size(); ++n) {
        QString line = lines.at(n);
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        if (line.trimmed().isEmpty() || line.startsWith(QLatin1Char(';')))
            continue;
        if (line.startsWith(QLatin1Char('['))) {
            if (!line.endsWith(QLatin1Char(']')) || line.size() < 3)
                return fail(n + 1, QStringLiteral("bad key line"));
            out.append(RegSection{line.mid(1, line.size() - 2), {}});
            continue;
        }
        if (out.isEmpty())
            return fail(n + 1, QStringLiteral("value before any key"));
        int at = 0;
        const auto name = unquoted(line, &at);
        if (!name || at >= line.size() || line.at(at) != QLatin1Char('='))
            return fail(n + 1, QStringLiteral("bad value line"));
        ++at;
        RegItem item;
        item.name = *name;
        const QString rest = line.mid(at);
        if (rest == QLatin1String("-")) {
            item.remove = true;
        } else if (rest.startsWith(QLatin1String("gvariant:"))) {
            item.gvariant = true;
            item.text = rest.mid(9);
        } else {
            const auto value = unquoted(line, &at);
            if (!value || at != line.size())
                return fail(n + 1, QStringLiteral("bad value line"));
            item.text = *value;
        }
        out.last().items.append(item);
    }
    return out;
}

}
