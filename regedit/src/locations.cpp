// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "locations.h"
#include "edits.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace Reg
{

namespace
{

// exact files and "<dir>/*.conf"-style folders; nothing else under /etc
const char *const s_systemExact[] = {
    "/etc/default/grub",
    "/etc/environment",
    "/etc/default/locale",
    "/etc/systemd/logind.conf",
    "/etc/systemd/journald.conf",
    "/etc/systemd/system.conf",
    "/etc/systemd/sleep.conf",
    "/etc/systemd/timesyncd.conf",
    "/etc/systemd/resolved.conf",
    "/etc/sddm.conf",
    "/etc/NetworkManager/NetworkManager.conf",
};

const char *const s_systemPatterns[] = {
    R"(^/etc/systemd/(logind|journald|system|sleep|timesyncd|resolved)\.conf\.d/[^/]+\.conf$)",
    R"(^/etc/sddm\.conf\.d/[^/]+\.conf$)",
    R"(^/etc/NetworkManager/conf\.d/[^/]+\.conf$)",
    R"(^/etc/snapper/configs/[^/]+$)",
    // /etc/xdg, one or two levels: kdeglobals, autostart is not settings
    R"(^/etc/xdg/(?!autostart/|menus/|systemd/)[^/]+(/[^/]+)?$)",
};

// shown under System but never written: every login and sudo reads it,
// and any variable there can make a root program load the user's code
const char *const s_shownOnly[] = {
    "/etc/environment",
};

const char *const s_information[] = {
    "/etc/os-release",
    "/etc/hostname",
};

// folders of ~/.config that hold databases, caches or secrets
const char *const s_userSkip[] = {
    "dconf", "chromium", "google-chrome", "BraveSoftware", "microsoft-edge", "Code", "pulse",
    "session", "systemd", "autostart", "menus", "fontconfig", "ibus", "libreoffice", "obs-studio",
    "discord", "Signal", "Element", "evolution", "goa-1.0", "keyrings",
};

QString underRoot(const QString &root, const char *path)
{
    return QDir::cleanPath(root + QLatin1Char('/') + QLatin1String(path));
}

}

QStringList systemFiles(const QString &root)
{
    QStringList out;
    for (const char *p : s_systemExact) {
        const QString path = underRoot(root, p);
        if (QFileInfo(path).isFile())
            out.append(path);
    }
    const char *const dirs[] = {
        "/etc/systemd/logind.conf.d", "/etc/systemd/journald.conf.d", "/etc/systemd/system.conf.d",
        "/etc/systemd/sleep.conf.d", "/etc/systemd/timesyncd.conf.d", "/etc/systemd/resolved.conf.d",
        "/etc/sddm.conf.d", "/etc/NetworkManager/conf.d", "/etc/snapper/configs",
    };
    for (const char *d : dirs) {
        const QString dir = underRoot(root, d);
        const auto names = QDir(dir).entryList(QDir::Files, QDir::Name);
        for (const QString &name : names) {
            const bool snapper = QString::fromLatin1(d).endsWith(QLatin1String("configs"));
            if ((snapper && !isPackageLeftover(name)) || name.endsWith(QLatin1String(".conf")))
                out.append(dir + QLatin1Char('/') + name);
        }
    }
    return out;
}

QStringList informationFiles(const QString &root)
{
    QStringList out;
    for (const char *p : s_information) {
        const QString path = underRoot(root, p);
        if (QFileInfo(path).isFile())
            out.append(path);
    }
    return out;
}

bool systemWritable(const QString &path)
{
    if (!path.startsWith(QLatin1Char('/')) || QDir::cleanPath(path) != path)
        return false;
    if (isPackageLeftover(path.section(QLatin1Char('/'), -1)))
        return false;
    for (const char *p : s_shownOnly) {
        if (path == QLatin1String(p))
            return false;
    }
    for (const char *p : s_systemExact) {
        if (path == QLatin1String(p))
            return true;
    }
    for (const char *p : s_systemPatterns) {
        if (QRegularExpression(QLatin1String(p)).match(path).hasMatch())
            return true;
    }
    return false;
}

bool isPackageLeftover(const QString &name)
{
    static const QRegularExpression leftover(QStringLiteral("\\.(dpkg-[a-z]+|ucf-[a-z]+|merge-error)$"));
    return leftover.match(name).hasMatch();
}

QString packageOf(const QString &path, const QString &root)
{
    const QDir info(root + QStringLiteral("/var/lib/dpkg/info"));
    const QByteArray want = QFile::encodeName(path);
    const QByteArray withFlag = want + QByteArrayLiteral(" ");
    const auto lists = info.entryList({QStringLiteral("*.conffiles")}, QDir::Files);
    for (const QString &name : lists) {
        QFile f(info.filePath(name));
        if (!f.open(QIODevice::ReadOnly))
            continue;
        while (!f.atEnd()) {
            // a line may end in " remove-on-upgrade" or " obsolete"
            const QByteArray line = f.readLine().trimmed();
            if (line == want || line.startsWith(withFlag))
                return name.chopped(10).section(QLatin1Char(':'), 0, 0);
        }
    }
    return QString();
}

Format formatFor(const QString &path)
{
    if (path.startsWith(QLatin1String("/etc/xdg/")))
        return Format::KConfig;
    if (path == QLatin1String("/etc/hostname"))
        return Format::Plain;
    if (path == QLatin1String("/etc/default/grub") || path == QLatin1String("/etc/environment")
        || path == QLatin1String("/etc/default/locale") || path == QLatin1String("/etc/os-release")
        || path.startsWith(QLatin1String("/etc/snapper/configs/")))
        return Format::Shell;
    if (path.startsWith(QLatin1String("/etc/")))
        return Format::Ini;
    // ~/.config: KConfig files end in rc or have no dot; the rest are plain ini
    const QString name = QFileInfo(path).fileName();
    if (name.endsWith(QLatin1String("rc")) || name == QLatin1String("kdeglobals") || !name.contains(QLatin1Char('.')))
        return Format::KConfig;
    return Format::Ini;
}

bool looksLikeSettings(const QByteArray &head)
{
    if (head.contains('\0'))
        return false;
    static const QRegularExpression entry(QStringLiteral("^[^=\\[{<\"]+="));
    const auto lines = head.split('\n');
    for (const QByteArray &raw : lines) {
        const QByteArray l = raw.trimmed();
        if (l.isEmpty() || l.startsWith('#') || l.startsWith(';'))
            continue;
        if (l.startsWith('[') && l.endsWith(']'))
            return true;
        return entry.match(QString::fromUtf8(l)).hasMatch();
    }
    return false;
}

QStringList userFiles(const QString &configDir, int depth)
{
    QStringList out;
    const QDir dir(configDir);
    const auto infos = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &i : infos) {
        if (i.isDir()) {
            if (depth <= 1 || i.isSymLink())
                continue;
            bool skip = false;
            for (const char *s : s_userSkip)
                skip = skip || i.fileName() == QLatin1String(s);
            if (skip)
                continue;
            const auto inner = userFiles(i.absoluteFilePath(), depth - 1);
            for (const QString &f : inner)
                out.append(i.fileName() + QLatin1Char('/') + f);
            continue;
        }
        if (i.size() > MaxFileSize || i.size() == 0 || isPackageLeftover(i.fileName()))
            continue;
        QFile f(i.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly))
            continue;
        if (looksLikeSettings(f.read(4096)))
            out.append(i.fileName());
    }
    return out;
}

}
