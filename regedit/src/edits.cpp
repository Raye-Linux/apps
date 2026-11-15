// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "edits.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>

#include <sys/stat.h>
#include <unistd.h>

namespace Reg
{

namespace
{

Outcome failed(const QString &error)
{
    Outcome o;
    o.error = error;
    return o;
}

void prune(const QString &root, int keep)
{
    QStringList files;
    QDirIterator it(root, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext())
        files.append(it.next());
    // <date>/<time>-<name>: the path sorts by age
    std::sort(files.begin(), files.end(), std::greater<>());
    for (int i = keep; i < files.size(); ++i)
        QFile::remove(files.at(i));
    const auto days = QDir(root).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &day : days)
        QDir(root).rmdir(day); // only goes when empty
}

QString keepCopy(const QString &path, const QByteArray &data, const QString &root, int keep, QString *error)
{
    const QDateTime now = QDateTime::currentDateTime();
    const QString day = root + QLatin1Char('/') + now.toString(QStringLiteral("yyyy-MM-dd"));
    if (!QDir().mkpath(day)) {
        *error = QStringLiteral("cannot make %1").arg(day);
        return QString();
    }
    QFile::setPermissions(root, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    QString name = path;
    name.replace(QLatin1Char('/'), QLatin1Char('%'));
    QString target = day + QLatin1Char('/') + now.toString(QStringLiteral("HHmmss.zzz")) + QLatin1Char('-') + name;
    for (int n = 2; QFileInfo::exists(target); ++n)
        target = day + QLatin1Char('/') + now.toString(QStringLiteral("HHmmss.zzz")) + QLatin1Char('-') + QString::number(n) + QLatin1Char('-') + name;
    QFile out(target);
    if (!out.open(QIODevice::WriteOnly | QIODevice::NewOnly) || out.write(data) != data.size()) {
        *error = QStringLiteral("cannot write %1").arg(target);
        return QString();
    }
    out.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    out.close();
    prune(root, keep);
    return target;
}

}

Outcome applyChange(const Change &change, const QString &backupRoot, int keepBackups)
{
    // the target of a link: a dotfile kept elsewhere stays a link
    QFileInfo info(change.path);
    const QString path = info.isSymLink() ? info.canonicalFilePath() : info.absoluteFilePath();
    info = QFileInfo(path);
    if (path.isEmpty() || !info.isFile())
        return failed(QStringLiteral("not a file: %1").arg(change.path));
    if (info.size() > MaxFileSize)
        return failed(QStringLiteral("too big: %1").arg(path));

    QFile in(path);
    if (!in.open(QIODevice::ReadOnly))
        return failed(in.errorString());
    const QByteArray before = in.readAll();
    in.close();

    SettingsFile file = SettingsFile::parse(before, change.format);
    Outcome o;
    if (const auto old = file.entry(change.group, change.key)) {
        o.hadValue = true;
        o.oldText = old->text;
    }
    QString error;
    const bool done = change.op == Change::Set ? file.set(change.group, change.key, change.text, &error)
                                               : file.remove(change.group, change.key, &error);
    if (!done)
        return failed(error);
    const QByteArray after = file.toBytes();
    if (after == before) {
        o.ok = true;
        return o;
    }

    o.backup = keepCopy(path, before, backupRoot, keepBackups, &error);
    if (o.backup.isEmpty())
        return failed(error);

    struct stat st {};
    const bool known = ::stat(QFile::encodeName(path).constData(), &st) == 0;
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly))
        return failed(out.errorString());
    out.setPermissions(QFile::permissions(path));
    if (known && ::fchown(out.handle(), st.st_uid, st.st_gid) != 0 && ::geteuid() == 0)
        return failed(QStringLiteral("cannot keep the owner of %1").arg(path));
    if (out.write(after) != after.size() || !out.commit())
        return failed(out.errorString());
    o.ok = true;
    o.changed = true;
    return o;
}

Change inverseOf(const Change &change, const Outcome &outcome)
{
    Change back = change;
    if (outcome.hadValue) {
        back.op = Change::Set;
        back.text = outcome.oldText;
    } else {
        back.op = Change::Remove;
        back.text.clear();
    }
    return back;
}

ValueType guessType(const QString &text, Format format)
{
    static const QRegularExpression number(QStringLiteral("^-?[0-9]+$"));
    const QString t = text.trimmed();
    if (t == QLatin1String("true") || t == QLatin1String("false"))
        return ValueType::YesNo;
    if (format != Format::KConfig && (t == QLatin1String("yes") || t == QLatin1String("no")))
        return ValueType::YesNo;
    if (number.match(t).hasMatch())
        return ValueType::Number;
    if (format == Format::KConfig && t.contains(QLatin1Char(',')))
        return ValueType::List;
    return ValueType::Text;
}

bool fitsType(const QString &oldText, const QString &newText, ValueType type)
{
    static const QRegularExpression number(QStringLiteral("^-?[0-9]+$"));
    switch (type) {
    case ValueType::Number:
        return number.match(newText.trimmed()).hasMatch();
    case ValueType::YesNo: {
        const QString o = oldText.trimmed();
        const QStringList words = (o == QLatin1String("yes") || o == QLatin1String("no"))
            ? QStringList{QStringLiteral("yes"), QStringLiteral("no")}
            : QStringList{QStringLiteral("true"), QStringLiteral("false")};
        return words.contains(newText);
    }
    default:
        return true;
    }
}

}
