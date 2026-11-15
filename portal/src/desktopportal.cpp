/*
 * SPDX-FileCopyrightText: 2016 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016 Jan Grulich <jgrulich@redhat.com>
 */

#include "desktopportal.h"
#include "desktopportal_debug.h"

#include "clipboard.h"
#include "remotedesktop.h"
#include "screencast.h"
#include "waylandintegration.h"

#if RAYE_PORTAL_ACCESS
#include "access/rayeaccessportal.h"
#endif

DesktopPortal::DesktopPortal(QObject *parent)
    : QObject(parent)
{
    // Raye: the Access prompt (permission questions) is added when its
    // folder is in the tree; raye.portal lists it only then.
#if RAYE_PORTAL_ACCESS
    new RayeAccessPortal(this);
#endif

    const QByteArray xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP");
    if (xdgCurrentDesktop.compare("KDE", Qt::CaseInsensitive) == 0) {
        m_screenCast = new ScreenCastPortal(this);
        m_remoteDesktop = new RemoteDesktopPortal(this);
        new ClipboardPortal(this);
        WaylandIntegration::init();
    } else {
        qCWarning(XdgDesktopPortalKdeDesktopPortal) << "Not a Plasma session: screen sharing is not offered";
    }
}

#include "moc_desktopportal.cpp"
