// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "pickerchoice.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QVariant>

using namespace Qt::StringLiterals;

namespace Raye
{

PickerChoice choosePicker(bool rayePickerBuilt, const QString &configuredStyle, const QByteArray &environment)
{
    const bool wantsDesktop = configuredStyle.trimmed().compare(u"kde"_s, Qt::CaseInsensitive) == 0
        || environment.trimmed().compare("kde", Qt::CaseInsensitive) == 0;
    if (rayePickerBuilt && !wantsDesktop) {
        return {u"org.raye.portal.picker"_s, u"SharePicker"_s, u"xdg-desktop-portal-raye"_s, true};
    }
    return {u"org.kde.xdgdesktopportal"_s, u"ScreenChooserDialog"_s, u"xdg-desktop-portal-kde"_s, false};
}

PickerChoice currentPicker()
{
    const KConfigGroup group(KSharedConfig::openConfig(u"xdg-desktop-portal-rayerc"_s), u"Picker"_s);
    return choosePicker(RAYE_PORTAL_PICKER, group.readEntry("Style", QString()), qgetenv("RAYE_PORTAL_PICKER"));
}

uint effectiveCursorMode(uint askedMode, const QVariant &showPointer)
{
    constexpr uint hidden = 1;
    // 0 is what an app that names no mode gets: no pointer either
    if (askedMode <= hidden || !showPointer.isValid() || showPointer.toBool()) {
        return askedMode;
    }
    return hidden;
}

} // namespace Raye
