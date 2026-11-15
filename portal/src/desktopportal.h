/*
 * SPDX-FileCopyrightText: 2016 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016 Jan Grulich <jgrulich@redhat.com>
 */

// Raye: trimmed to the portals this backend serves (ScreenCast,
// RemoteDesktop and the Clipboard of remote desktop sessions). The
// desktop's own backend keeps every other portal.

#ifndef XDG_DESKTOP_PORTAL_KDE_DESKTOP_PORTAL_H
#define XDG_DESKTOP_PORTAL_KDE_DESKTOP_PORTAL_H

#include <QDBusContext>
#include <QObject>

class ScreenCastPortal;
class RemoteDesktopPortal;

class DesktopPortal : public QObject, public QDBusContext
{
    Q_OBJECT
public:
    explicit DesktopPortal(QObject *parent = nullptr);

private:
    ScreenCastPortal *m_screenCast = nullptr;
    RemoteDesktopPortal *m_remoteDesktop = nullptr;
};

#endif // XDG_DESKTOP_PORTAL_KDE_DESKTOP_PORTAL_H
