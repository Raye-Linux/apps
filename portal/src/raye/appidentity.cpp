// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "appidentity.h"

#include <KConfigGroup>
#include <KDesktopFile>
#include <KLocalizedString>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

namespace Raye
{

static constexpr auto unknownIcon = "application-x-executable";

bool isPlausibleAppId(const QString &appId)
{
    // letters, digits, dots, dashes and underscores; no "/" or "..", so
    // the id can never name a file outside the applications folders
    static const QRegularExpression valid(u"^[A-Za-z0-9_][A-Za-z0-9_.-]{0,254}$"_s);
    return valid.match(appId).hasMatch() && !appId.contains(u".."_s);
}

static QString findDesktopFile(const QString &fileName, const QStringList &appDirs)
{
    if (appDirs.isEmpty()) {
        return QStandardPaths::locate(QStandardPaths::ApplicationsLocation, fileName);
    }
    for (const QString &dir : appDirs) {
        const QString path = QDir(dir).filePath(fileName);
        if (QFileInfo(path).isFile()) {
            return path;
        }
    }
    return {};
}

AppIdentity appIdentity(const QString &appId, const QStringList &appDirs)
{
    AppIdentity identity;
    identity.id = appId;
    identity.name = i18nc("@label an app whose name is not known", "An app");
    identity.iconName = QString::fromLatin1(unknownIcon);

    if (!isPlausibleAppId(appId)) {
        return identity;
    }

    QString path = findDesktopFile(appId + u".desktop"_s, appDirs);
    if (path.isEmpty()) {
        return identity;
    }

    const KDesktopFile file(path);
    const KConfigGroup group = file.desktopGroup();
    if (group.readEntry("Type", QString()) != u"Application"_s && !group.readEntry("Type", QString()).isEmpty()) {
        return identity;
    }
    // Plasma and the GNOME apps both read X-GNOME-FullName first
    QString name = group.readEntry("X-GNOME-FullName", QString());
    if (name.isEmpty()) {
        name = file.readName();
    }
    if (name.trimmed().isEmpty()) {
        return identity;
    }

    identity.name = name.trimmed();
    identity.known = true;
    const QString icon = file.readIcon();
    if (!icon.isEmpty()) {
        identity.iconName = icon;
    }
    return identity;
}

} // namespace Raye
