// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "helperargs.h"
#include "locations.h"

#include <QFile>
#include <QRegularExpression>

#include <sys/stat.h>

namespace Reg
{

bool isCodeKey(const QString &path, const QString &key)
{
    const bool systemd = path == QLatin1String("/etc/systemd/system.conf")
        || path.startsWith(QLatin1String("/etc/systemd/system.conf.d/"));
    if (systemd && key.endsWith(QLatin1String("Environment")))
        return true;
    if (path == QLatin1String("/etc/default/grub") && key.startsWith(QLatin1String("GRUB_CMDLINE_")))
        return true;
    const bool sddm = path == QLatin1String("/etc/sddm.conf") || path.startsWith(QLatin1String("/etc/sddm.conf.d/"));
    static const QRegularExpression sddmCode(QStringLiteral("(Command|Path|Arguments|Dir|Environment)$"));
    if (sddm && sddmCode.match(key).hasMatch())
        return true;
    if (path.startsWith(QLatin1String("/etc/snapper/configs/"))
        && (key == QLatin1String("SUBVOLUME") || key.startsWith(QLatin1String("ALLOW_")) || key == QLatin1String("FSTYPE")))
        return true;
    return false;
}

QStringList helperArguments(const Change &change)
{
    return {change.op == Change::Set ? QStringLiteral("set") : QStringLiteral("remove"), change.path,
            change.group.isEmpty() ? QStringLiteral("-") : change.group.join(GroupSeparator), change.key};
}

std::optional<Change> parseHelperArguments(const QStringList &args, const QByteArray &input, QString *error)
{
    if (args.size() != 4 || (args.at(0) != QLatin1String("set") && args.at(0) != QLatin1String("remove"))) {
        *error = QStringLiteral("usage: set|remove <path> <group> <key>");
        return std::nullopt;
    }
    Change c;
    c.op = args.at(0) == QLatin1String("set") ? Change::Set : Change::Remove;
    c.path = args.at(1);
    if (!systemWritable(c.path)) {
        *error = QStringLiteral("not a file this helper writes: %1").arg(c.path);
        return std::nullopt;
    }
    c.format = formatFor(c.path);
    if (c.format == Format::Plain) {
        *error = QStringLiteral("read only: %1").arg(c.path);
        return std::nullopt;
    }
    if (args.at(2) != QLatin1String("-"))
        c.group = args.at(2).split(GroupSeparator);
    c.key = args.at(3);
    if (isCodeKey(c.path, c.key)) {
        *error = QStringLiteral("this key is not changed here: %1").arg(c.key);
        return std::nullopt;
    }
    if (c.op == Change::Set) {
        if (input.size() > MaxFileSize) {
            *error = QStringLiteral("value too long");
            return std::nullopt;
        }
        c.text = QString::fromUtf8(input);
    }
    return c;
}

QString refuseSystemPath(const QString &listed, uid_t owner, const QString &root)
{
    if (!systemWritable(listed))
        return QStringLiteral("not on the list: %1").arg(listed);
    const QString path = root + listed;
    struct stat st {};
    if (::lstat(QFile::encodeName(path).constData(), &st) != 0)
        return QStringLiteral("missing: %1").arg(path);
    if (S_ISLNK(st.st_mode))
        return QStringLiteral("a link: %1").arg(path);
    if (!S_ISREG(st.st_mode))
        return QStringLiteral("not a regular file: %1").arg(path);
    if (st.st_uid != owner)
        return QStringLiteral("not owned by root: %1").arg(path);
    if (st.st_size > MaxFileSize)
        return QStringLiteral("too big: %1").arg(path);
    const QString package = packageOf(listed, root);
    if (!package.isEmpty())
        return QStringLiteral("part of the %1 package: %2").arg(package, path);
    // the editor only shows files that read as settings
    QFile f(path);
    if (st.st_size > 0 && (!f.open(QIODevice::ReadOnly) || !looksLikeSettings(f.read(4096))))
        return QStringLiteral("not a settings file: %1").arg(path);
    // HKLM\Defaults is /etc/xdg read the way ~/.config is: same list here
    if (listed.startsWith(QLatin1String("/etc/xdg/"))
        && !userFiles(root + QStringLiteral("/etc/xdg"), 2).contains(listed.mid(9)))
        return QStringLiteral("not shown in the editor: %1").arg(path);
    // a folder on the way that is a link could lead anywhere
    QString dir = path.section(QLatin1Char('/'), 0, -2);
    while (dir.size() > root.size() + 1) {
        if (::lstat(QFile::encodeName(dir).constData(), &st) != 0 || S_ISLNK(st.st_mode))
            return QStringLiteral("a link on the way: %1").arg(dir);
        dir = dir.section(QLatin1Char('/'), 0, -2);
    }
    return QString();
}

}
