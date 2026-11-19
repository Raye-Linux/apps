// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "windowsprocess.h"

#include <QSet>

namespace Raye::TaskManager
{

namespace
{
bool isLayerBinary(const QString &exe)
{
    if (exe.isEmpty()) {
        return false;
    }
    const QString name = exe.section(QLatin1Char('/'), -1);
    static const QSet<QString> loaders{QStringLiteral("wine"), QStringLiteral("wine64"), QStringLiteral("wine-preloader"),
                                       QStringLiteral("wine64-preloader"), QStringLiteral("wineserver"),
                                       QStringLiteral("wineserver64")};
    return loaders.contains(name);
}

// helpers the layer starts by name before they have a path
bool isHelperName(const QString &file)
{
    static const QSet<QString> helpers{QStringLiteral("start.exe"), QStringLiteral("conhost.exe"),
                                       QStringLiteral("services.exe"), QStringLiteral("winedevice.exe"),
                                       QStringLiteral("plugplay.exe"), QStringLiteral("explorer.exe"),
                                       QStringLiteral("rpcss.exe"), QStringLiteral("svchost.exe"),
                                       QStringLiteral("wineboot.exe"), QStringLiteral("winemenubuilder.exe"),
                                       QStringLiteral("winedbg.exe"), QStringLiteral("rundll32.exe"),
                                       QStringLiteral("lsass.exe"), QStringLiteral("tabtip.exe")};
    return helpers.contains(file.toLower());
}
}

WindowsProcess windowsProcess(const QString &exe, const QByteArray &cmdline)
{
    WindowsProcess out;
    if (!isLayerBinary(exe)) {
        return out;
    }
    out.role = WindowsRole::Support;
    if (exe.endsWith(QLatin1String("/wineserver")) || exe.endsWith(QLatin1String("/wineserver64"))) {
        return out;
    }
    const int end = cmdline.indexOf('\0');
    const QString first = QString::fromUtf8(end < 0 ? cmdline : cmdline.left(end));
    if (first.isEmpty()) {
        return out;
    }
    // "X:\windows\..." is the layer's own folder
    if (first.size() > 3 && first.at(1) == QLatin1Char(':') && first.mid(2, 9).compare(QLatin1String("\\windows\\"), Qt::CaseInsensitive) == 0) {
        return out;
    }
    QString file = first;
    const int cut = qMax(file.lastIndexOf(QLatin1Char('\\')), file.lastIndexOf(QLatin1Char('/')));
    file = file.mid(cut + 1);
    if (file.isEmpty() || (cut < 0 && isHelperName(file))) {
        return out;
    }
    // the loader itself, before it has handed over to a Windows program
    if (isLayerBinary(first)) {
        return out;
    }
    out.role = WindowsRole::Program;
    out.file = file;
    return out;
}

QString windowsProgramAppId(const QString &file)
{
    return QStringLiteral("windows:") + file;
}

QString windowsProgramFile(const QString &appId)
{
    return appId.startsWith(QLatin1String("windows:")) ? appId.mid(8) : QString();
}

QString windowsProgramName(const QString &file)
{
    return file.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive) ? file.chopped(4) : file;
}

bool isGenericWindowsLauncher(const QString &appId)
{
    return appId == QLatin1String("raye-run-windows") || appId == QLatin1String("wine")
        || appId == QLatin1String("org.raye.windows.package");
}

} // namespace Raye::TaskManager
