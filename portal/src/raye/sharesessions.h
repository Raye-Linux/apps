// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// What is being shared right now, for the sharing indicator and the
// taskbar: org.raye.ScreenShare on the session bus, object
// /org/raye/ScreenShare.
//
//   property  Sessions        aa{sv}  one map per sharing session
//   method    Stop(s handle)  -> b    ends that session (the app is told)
//   method    StopAll()
//   signal    SessionsChanged(aa{sv})
//
// A session map holds: handle (s), app_id (s), app_name (s),
// app_icon (s), kind (s: screen, window, area, workspace, virtual or
// mixed), target (s: the name of what is shared, the first one when
// there are several), targets (aa{sv}: kind, name, id, app_id, node),
// remote_control (b: the app may also move the pointer and type),
// started (x: milliseconds since the epoch).

#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QObject>
#include <QVariantMap>

#include <functional>

namespace Raye
{

struct ShareTarget {
    QString kind; // screen, window, area, workspace, virtual
    QString name; // "Screen 1", a window title...
    QString id; // output id, or the window's uuid
    QString appId; // for a window: the app that owns it
    uint node = 0; // PipeWire node of the stream
};

struct ShareInfo {
    QString handle;
    QString appId;
    QString appName;
    QString appIcon;
    QList<ShareTarget> targets;
    bool remoteControl = false;
    qint64 started = 0;
};

// Summary kind of a session: the targets' kind when they all agree,
// "mixed" otherwise, "screen" when nothing is known.
QString summaryKind(const QList<ShareTarget> &targets);

QVariantMap toVariantMap(const ShareInfo &info);

class ShareSessions : public QObject
{
    Q_OBJECT
public:
    using StopFunction = std::function<void()>;

    explicit ShareSessions(QObject *parent = nullptr);

    // The one list of the backend.
    static ShareSessions *instance();

    // Adds or replaces the session with this handle. stop ends it; it
    // is called at most once, by stop() or stopAll().
    void publish(const ShareInfo &info, StopFunction stop);
    // The session ended by itself.
    void withdraw(const QString &handle);

    QList<QVariantMap> sessions() const;
    int count() const;

    bool stop(const QString &handle);
    void stopAll();

    // Exports the list on a bus: the object, and the name
    // org.raye.ScreenShare when it is free.
    bool exportOn(QDBusConnection bus);

Q_SIGNALS:
    void changed();

private:
    struct Entry {
        ShareInfo info;
        StopFunction stop;
    };
    QList<Entry> m_entries;
};

class ShareSessionsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.raye.ScreenShare")
    Q_PROPERTY(QList<QVariantMap> Sessions READ sessions)
public:
    ShareSessionsAdaptor(ShareSessions *parent, const QDBusConnection &bus, const QString &path);

    QList<QVariantMap> sessions() const;

public Q_SLOTS:
    bool Stop(const QString &handle);
    void StopAll();

Q_SIGNALS:
    void SessionsChanged(const QList<QVariantMap> &sessions);

private:
    void announce();
    ShareSessions *const m_sessions;
    QDBusConnection m_bus;
    const QString m_path;
};

inline constexpr auto shareServiceName = "org.raye.ScreenShare";
inline constexpr auto shareObjectPath = "/org/raye/ScreenShare";

} // namespace Raye
