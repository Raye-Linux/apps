// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "raye/sharesessions.h"

#include <QDBusArgument>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QSignalSpy>
#include <QTest>

using namespace Raye;
using namespace Qt::StringLiterals;

static ShareInfo discordWindow()
{
    ShareInfo info;
    info.handle = u"/org/freedesktop/portal/desktop/session/1_42/a"_s;
    info.appId = u"com.discordapp.Discord"_s;
    info.appName = u"Discord"_s;
    info.appIcon = u"com.discordapp.Discord"_s;
    info.targets = {{u"window"_s, u"notes.txt - Notepad"_s, u"uuid-1"_s, u"org.raye.notepad"_s, 51}};
    info.started = 1000;
    return info;
}

static ShareInfo browserScreens()
{
    ShareInfo info;
    info.handle = u"/org/freedesktop/portal/desktop/session/1_43/b"_s;
    info.appId = u"org.mozilla.firefox"_s;
    info.appName = u"Firefox"_s;
    info.targets = {{u"screen"_s, u"Screen 1"_s, u"DP-1"_s, {}, 60}, {u"screen"_s, u"Screen 2"_s, u"HDMI-A-1"_s, {}, 61}};
    return info;
}

// a map read back from D-Bus holds QDBusArgument for nested containers
static QList<QVariantMap> mapsFrom(const QVariant &value)
{
    if (value.canConvert<QDBusArgument>()) {
        return qdbus_cast<QList<QVariantMap>>(value.value<QDBusArgument>());
    }
    return value.value<QList<QVariantMap>>();
}

// receives the signals a watcher of the list gets
class Catcher : public QObject
{
    Q_OBJECT
public:
    QList<QVariantMap> announced;
    int propertiesChanged = 0;

public Q_SLOTS:
    void sessionsChanged(const QDBusMessage &message)
    {
        announced = mapsFrom(message.arguments().value(0));
    }
    void changed(const QDBusMessage &message)
    {
        if (message.arguments().value(0).toString() == u"org.raye.ScreenShare"_s) {
            ++propertiesChanged;
        }
    }
};

class TestShareSessions : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void summaryKinds()
    {
        QCOMPARE(summaryKind({}), u"screen"_s);
        QCOMPARE(summaryKind(discordWindow().targets), u"window"_s);
        QCOMPARE(summaryKind(browserScreens().targets), u"screen"_s);
        QList<ShareTarget> mixed = browserScreens().targets;
        mixed.append(discordWindow().targets.first());
        QCOMPARE(summaryKind(mixed), u"mixed"_s);
    }

    void mapOfASession()
    {
        const QVariantMap map = toVariantMap(discordWindow());
        QCOMPARE(map.value(u"handle"_s).toString(), discordWindow().handle);
        QCOMPARE(map.value(u"app_id"_s).toString(), u"com.discordapp.Discord"_s);
        QCOMPARE(map.value(u"app_name"_s).toString(), u"Discord"_s);
        QCOMPARE(map.value(u"kind"_s).toString(), u"window"_s);
        QCOMPARE(map.value(u"target"_s).toString(), u"notes.txt - Notepad"_s);
        QCOMPARE(map.value(u"remote_control"_s).toBool(), false);
        QCOMPARE(map.value(u"started"_s).toLongLong(), 1000);
        const QList<QVariantMap> targets = map.value(u"targets"_s).value<QList<QVariantMap>>();
        QCOMPARE(targets.size(), 1);
        QCOMPARE(targets.first().value(u"id"_s).toString(), u"uuid-1"_s);
        QCOMPARE(targets.first().value(u"app_id"_s).toString(), u"org.raye.notepad"_s);
        QCOMPARE(targets.first().value(u"node"_s).toUInt(), 51u);
    }

    void publishReplaceWithdraw()
    {
        ShareSessions list;
        QSignalSpy changed(&list, &ShareSessions::changed);
        list.publish(discordWindow(), {});
        list.publish(browserScreens(), {});
        QCOMPARE(list.count(), 2);
        QCOMPARE(changed.count(), 2);

        // same handle again: replaced, not added
        ShareInfo again = discordWindow();
        again.targets.first().name = u"Other title"_s;
        list.publish(again, {});
        QCOMPARE(list.count(), 2);
        QCOMPARE(list.sessions().first().value(u"target"_s).toString(), u"Other title"_s);

        list.withdraw(discordWindow().handle);
        QCOMPARE(list.count(), 1);
        QCOMPARE(changed.count(), 4);
        // unknown handle: nothing happens
        list.withdraw(u"/nope"_s);
        QCOMPARE(changed.count(), 4);
    }

    void startTimeIsSet()
    {
        ShareSessions list;
        ShareInfo info = browserScreens();
        info.started = 0;
        list.publish(info, {});
        QVERIFY(list.sessions().first().value(u"started"_s).toLongLong() > 0);
    }

    void stopCallsTheSessionOnce()
    {
        ShareSessions list;
        int stops = 0;
        // stopping a session closes it, which withdraws it again
        list.publish(discordWindow(), [&] {
            ++stops;
            list.withdraw(discordWindow().handle);
        });
        QVERIFY(list.stop(discordWindow().handle));
        QCOMPARE(stops, 1);
        QCOMPARE(list.count(), 0);
        QVERIFY(!list.stop(discordWindow().handle));
        QCOMPARE(stops, 1);
    }

    void stopAll()
    {
        ShareSessions list;
        int stops = 0;
        list.publish(discordWindow(), [&] {
            ++stops;
        });
        list.publish(browserScreens(), [&] {
            ++stops;
        });
        QSignalSpy changed(&list, &ShareSessions::changed);
        list.stopAll();
        QCOMPARE(stops, 2);
        QCOMPARE(list.count(), 0);
        QCOMPARE(changed.count(), 1);
        list.stopAll();
        QCOMPARE(changed.count(), 1);
    }

    // On a real session bus (the test runs under dbus-run-session): the
    // list, its signal and Stop, as a watcher in another process sees them.
    void onTheBus()
    {
        QDBusConnection bus = QDBusConnection::sessionBus();
        if (!bus.isConnected()) {
            QSKIP("no session bus");
        }
        ShareSessions list;
        int stops = 0;
        list.publish(discordWindow(), [&] {
            ++stops;
        });
        QVERIFY(list.exportOn(bus));
        QVERIFY(bus.interface()->isServiceRegistered(QString::fromLatin1(shareServiceName)));

        QDBusConnection watcher = QDBusConnection::connectToBus(QDBusConnection::SessionBus, u"watcher"_s);
        QVERIFY(watcher.isConnected());

        QDBusMessage get = QDBusMessage::createMethodCall(QString::fromLatin1(shareServiceName),
                                                          QString::fromLatin1(shareObjectPath),
                                                          u"org.freedesktop.DBus.Properties"_s,
                                                          u"Get"_s);
        get << u"org.raye.ScreenShare"_s << u"Sessions"_s;
        QDBusPendingReply<QDBusVariant> reply = watcher.asyncCall(get);
        QTRY_VERIFY(reply.isFinished());
        QVERIFY2(reply.isValid(), qPrintable(reply.error().message()));
        const QList<QVariantMap> sessions = mapsFrom(reply.value().variant());
        QCOMPARE(sessions.size(), 1);
        QCOMPARE(sessions.first().value(u"app_name"_s).toString(), u"Discord"_s);
        QCOMPARE(sessions.first().value(u"kind"_s).toString(), u"window"_s);

        // a change is announced both ways
        Catcher catcher;
        QList<QVariantMap> &announced = catcher.announced;
        int &propertiesChanged = catcher.propertiesChanged;
        QVERIFY(watcher.connect(QString(), QString::fromLatin1(shareObjectPath), u"org.raye.ScreenShare"_s, u"SessionsChanged"_s,
                                &catcher, SLOT(sessionsChanged(QDBusMessage))));
        QVERIFY(watcher.connect(QString(), QString::fromLatin1(shareObjectPath), u"org.freedesktop.DBus.Properties"_s, u"PropertiesChanged"_s,
                                &catcher, SLOT(changed(QDBusMessage))));
        list.publish(browserScreens(), {});
        QTRY_COMPARE(announced.size(), 2);
        QTRY_VERIFY(propertiesChanged >= 1);

        // Stop from the watcher ends the session
        QDBusMessage stop = QDBusMessage::createMethodCall(QString::fromLatin1(shareServiceName),
                                                           QString::fromLatin1(shareObjectPath),
                                                           u"org.raye.ScreenShare"_s,
                                                           u"Stop"_s);
        stop << discordWindow().handle;
        QDBusPendingReply<bool> stopped = watcher.asyncCall(stop);
        QTRY_VERIFY(stopped.isFinished());
        QVERIFY(stopped.isValid());
        QVERIFY(stopped.value());
        QCOMPARE(stops, 1);
        QCOMPARE(list.count(), 1);
        QTRY_COMPARE(announced.size(), 1);

        QDBusConnection::disconnectFromBus(u"watcher"_s);
        bus.unregisterObject(QString::fromLatin1(shareObjectPath));
        bus.interface()->unregisterService(QString::fromLatin1(shareServiceName));
    }
};

QTEST_GUILESS_MAIN(TestShareSessions)
#include "test_sharesessions.moc"
