// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "sharehook.h"

#include "appidentity.h"
#include "sharesessions.h"

#include "../remotedesktop.h"
#include "../screencast.h"

#include <QPointer>

using namespace Qt::StringLiterals;

namespace Raye
{

static QString kindOfStream(const WaylandIntegration::Stream &stream)
{
    switch (stream.map.value(u"source_type"_s).toUInt()) {
    case ScreenCastPortal::Window:
        return u"window"_s;
    case ScreenCastPortal::Virtual:
        return u"virtual"_s;
    default:
        return u"screen"_s;
    }
}

void publishShare(ScreenCastSession *session)
{
    if (!session) {
        return;
    }
    const auto streams = session->streams();

    // what the picker said was chosen, in stream order; streams started
    // without the picker (remote desktop) are named by their kind only
    QList<ShareTarget> targets = session->shareTargets();
    if (targets.size() != streams.size()) {
        targets.clear();
        for (const auto &stream : streams) {
            targets.append({kindOfStream(stream), {}, {}, {}, 0});
        }
    }
    for (qsizetype i = 0; i < targets.size(); ++i) {
        targets[i].node = streams.at(i).nodeId;
    }

    const AppIdentity app = appIdentity(session->appId());
    ShareInfo info;
    info.handle = session->handle();
    info.appId = session->appId();
    info.appName = app.name;
    info.appIcon = app.iconName;
    info.targets = targets;
    info.remoteControl = qobject_cast<RemoteDesktopSession *>(session) != nullptr;

    QPointer<ScreenCastSession> guarded(session);
    ShareSessions::instance()->publish(info, [guarded] {
        if (guarded) {
            guarded->close();
        }
    });

    if (!session->property("rayeSharePublished").toBool()) {
        session->setProperty("rayeSharePublished", true);
        const QString handle = session->handle();
        QObject::connect(session, &Session::closed, ShareSessions::instance(), [handle] {
            ShareSessions::instance()->withdraw(handle);
        });
        QObject::connect(session, &QObject::destroyed, ShareSessions::instance(), [handle] {
            ShareSessions::instance()->withdraw(handle);
        });
    }
}

} // namespace Raye
