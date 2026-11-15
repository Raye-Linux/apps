// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

#include <optional>

typedef struct _DConfClient DConfClient;

// The user's dconf database: what GTK apps, Firefox and the GNOME parts
// keep. Reads go to the database file, writes through dconf-service.
namespace Reg
{

struct DconfValue {
    QString type; // GVariant type string: "b", "s", "as", "(ii)"...
    QString text; // GVariant text: true, 'Inter 10', ['a', 'b']
};

// the text a user typed, read as a value of this type; nullopt and
// *error when it does not parse as that type
std::optional<QString> dconfCanonical(const QString &text, const QString &type, QString *error = nullptr);
// a typed-in value for a NEW key: "Text" "Number" "YesNo" "List"
std::optional<DconfValue> dconfNewValue(const QString &text, const QString &kind, QString *error = nullptr);
// the type GVariant reads this text as, with no type given ('x' -> s, 5 -> i)
std::optional<QString> dconfGuessType(const QString &text);
// the GVariant text of a string, quoted the way GVariant reads it back
QString dconfQuote(const QString &text);
// a string value without its quotes, for display ("'Inter 10'" -> "Inter 10")
QString dconfDisplay(const DconfValue &value);

class DconfStore
{
public:
    DconfStore();
    ~DconfStore();
    DconfStore(const DconfStore &) = delete;
    DconfStore &operator=(const DconfStore &) = delete;

    // children of a directory ("/" or "/org/gnome/"): dirs end with "/"
    QStringList list(const QString &dir) const;
    std::optional<DconfValue> read(const QString &key) const;
    bool write(const QString &key, const DconfValue &value, QString *error = nullptr);
    bool reset(const QString &key, QString *error = nullptr);
    bool writable(const QString &key) const;

private:
    DConfClient *m_client = nullptr;
};

}
