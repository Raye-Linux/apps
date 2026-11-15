// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "sharesessions.h"

#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDateTime>

using namespace Qt::StringLiterals;

namespace Raye
{

QString summaryKind(const QList<ShareTarget> &targets)
{
    if (targets.isEmpty()) {
        return u"screen"_s;
    }
    const QString first = targets.first().kind;
    for (const ShareTarget &target : targets) {
        if (target.kind != first) {
            return u"mixed"_s;
        }
    }
    return first;
}

QVariantMap toVariantMap(const ShareInfo &info)
{
    QList<QVariantMap> targets;
    for (const ShareTarget &target : info.targets) {
        targets.append({
            {u"kind"_s, target.kind},
            {u"name"_s, target.name},
            {u"id"_s, target.id},
            {u"app_id"_s, target.appId},
            {u"node"_s, target.node},
        });
    }
    return {
        {u"handle"_s, info.handle},
        {u"app_id"_s, info.appId},
        {u"app_name"_s, info.appName},
        {u"app_icon"_s, info.appIcon},
        {u"kind"_s, summaryKind(info.targets)},
        {u"target"_s, info.targets.isEmpty() ? QString() : info.targets.first().name},
        {u"targets"_s, QVariant::fromValue(targets)},
        {u"remote_control"_s, info.remoteControl},
        {u"started"_s, info.started},
    };
}

ShareSessions::ShareSessions(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<QList<QVariantMap>>();
}

ShareSessions *ShareSessions::instance()
{
    static ShareSessions *list = new ShareSessions;
    return list;
}

void ShareSessions::publish(const ShareInfo &info, StopFunction stop)
{
    ShareInfo copy = info;
    if (copy.started == 0) {
        copy.started = QDateTime::currentMSecsSinceEpoch();
    }
    for (Entry &entry : m_entries) {
        if (entry.info.handle == copy.handle) {
            entry.info = copy;
            entry.stop = std::move(stop);
            Q_EMIT changed();
            return;
        }
    }
    m_entries.append({copy, std::move(stop)});
    Q_EMIT changed();
}

void ShareSessions::withdraw(const QString &handle)
{
    const auto removed = m_entries.removeIf([&handle](const Entry &entry) {
        return entry.info.handle == handle;
    });
    if (removed > 0) {
        Q_EMIT changed();
    }
}

QList<QVariantMap> ShareSessions::sessions() const
{
    QList<QVariantMap> list;
    list.reserve(m_entries.size());
    for (const Entry &entry : m_entries) {
        list.append(toVariantMap(entry.info));
    }
    return list;
}

int ShareSessions::count() const
{
    return m_entries.size();
}

bool ShareSessions::stop(const QString &handle)
{
    for (qsizetype i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).info.handle != handle) {
            continue;
        }
        // take it out first: stopping closes the session, which may
        // call withdraw() again
        const StopFunction stopFunction = m_entries.takeAt(i).stop;
        Q_EMIT changed();
        if (stopFunction) {
            stopFunction();
        }
        return true;
    }
    return false;
}

void ShareSessions::stopAll()
{
    const QList<Entry> entries = std::exchange(m_entries, {});
    if (entries.isEmpty()) {
        return;
    }
    Q_EMIT changed();
    for (const Entry &entry : entries) {
        if (entry.stop) {
            entry.stop();
        }
    }
}

bool ShareSessions::exportOn(QDBusConnection bus)
{
    const QString path = QString::fromLatin1(shareObjectPath);
    new ShareSessionsAdaptor(this, bus, path);
    if (!bus.registerObject(path, this, QDBusConnection::ExportAdaptors)) {
        return false;
    }
    // the name is a convenience for watchers; the object is also reachable
    // on the backend's own name
    bus.interface()->registerService(QString::fromLatin1(shareServiceName),
                                     QDBusConnectionInterface::DontQueueService,
                                     QDBusConnectionInterface::DontAllowReplacement);
    return true;
}

ShareSessionsAdaptor::ShareSessionsAdaptor(ShareSessions *parent, const QDBusConnection &bus, const QString &path)
    : QDBusAbstractAdaptor(parent)
    , m_sessions(parent)
    , m_bus(bus)
    , m_path(path)
{
    connect(parent, &ShareSessions::changed, this, &ShareSessionsAdaptor::announce);
}

QList<QVariantMap> ShareSessionsAdaptor::sessions() const
{
    return m_sessions->sessions();
}

bool ShareSessionsAdaptor::Stop(const QString &handle)
{
    return m_sessions->stop(handle);
}

void ShareSessionsAdaptor::StopAll()
{
    m_sessions->stopAll();
}

void ShareSessionsAdaptor::announce()
{
    const QList<QVariantMap> list = m_sessions->sessions();
    Q_EMIT SessionsChanged(list);

    QDBusMessage message = QDBusMessage::createSignal(m_path, u"org.freedesktop.DBus.Properties"_s, u"PropertiesChanged"_s);
    message << u"org.raye.ScreenShare"_s << QVariantMap{{u"Sessions"_s, QVariant::fromValue(list)}} << QStringList();
    m_bus.send(message);
}

} // namespace Raye

#include "moc_sharesessions.cpp"
