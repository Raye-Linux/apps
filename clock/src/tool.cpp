// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// raye-clock-tool: the clock's part that runs without its window.
//
//   schedule   arms the user timer for the next alarm or timer (or starts
//              the ring service when one is already due)
//   ring       what that timer starts: rings what is due with a sound and
//              a notification (Snooze, Dismiss), and arms the next one.
//              It ends when the last ring on screen is answered.

#include "clockstore.h"

#include <KLocalizedString>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QFileSystemWatcher>
#include <QLocale>
#include <QProcess>
#include <QSet>
#include <QSettings>
#include <QSoundEffect>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <cstdio>

namespace
{
const QString kService = QStringLiteral("org.freedesktop.Notifications");
const QString kPath = QStringLiteral("/org/freedesktop/Notifications");
// the sound stops after this long; the notification stays
constexpr int kRingMaxMs = 10 * 60 * 1000;
// from soft to full over this long
constexpr int kRampMs = 20 * 1000;

QList<Alarm> loadAlarms()
{
    return Alarms::fromJson(ClockStore::read(ClockStore::alarmsPath()));
}

QList<Countdown> loadTimers()
{
    return Countdowns::fromJson(ClockStore::read(ClockStore::timersPath()));
}

RingState loadState()
{
    return RingState::fromJson(ClockStore::read(ClockStore::statePath()));
}

QString soundFile(const QString &name)
{
    for (const QString &n : {name, QStringLiteral("glow")}) {
        const QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                    QStringLiteral("raye-clock/sounds/%1.wav").arg(n));
        if (!path.isEmpty())
            return path;
    }
    return {};
}

QString timerSound()
{
    QSettings s(ClockStore::settingsPath(), QSettings::IniFormat);
    return s.value(QStringLiteral("Timers/sound"), QStringLiteral("chimes")).toString();
}

QString timeText(const QDateTime &t)
{
    return QLocale().toString(t.toLocalTime().time(), QLocale::ShortFormat);
}

QString lengthText(qint64 ms)
{
    const qint64 minutes = ms / 60000;
    const qint64 seconds = ms / 1000 % 60;
    if (minutes >= 60)
        return i18ncp("@info timer length", "%1 hour", "%1 hours", minutes / 60)
            + (minutes % 60 ? QLatin1Char(' ') + i18ncp("@info timer length", "%1 min", "%1 min", minutes % 60) : QString());
    if (minutes > 0)
        return i18ncp("@info timer length", "%1 min", "%1 min", minutes)
            + (seconds ? QLatin1Char(' ') + i18ncp("@info timer length", "%1 sec", "%1 sec", seconds) : QString());
    return i18ncp("@info timer length", "%1 sec", "%1 sec", seconds);
}

class Ringer : public QObject
{
    Q_OBJECT

public:
    Ringer()
    {
        auto bus = QDBusConnection::sessionBus();
        bus.connect(kService, kPath, kService, QStringLiteral("ActionInvoked"), this, SLOT(actionInvoked(uint, QString)));
        bus.connect(kService, kPath, kService, QStringLiteral("NotificationClosed"), this, SLOT(closed(uint, uint)));
        m_next.setSingleShot(true);
        connect(&m_next, &QTimer::timeout, this, &Ringer::showDue);
        m_quiet.setSingleShot(true);
        connect(&m_quiet, &QTimer::timeout, this, [this] { m_sound.stop(); });
        m_ramp.setInterval(500);
        connect(&m_ramp, &QTimer::timeout, this, [this] {
            const qreal v = qMin(1.0, m_sound.volume() + 0.75 * 500 / kRampMs);
            m_sound.setVolume(v);
            if (v >= 1.0)
                m_ramp.stop();
        });
        m_sound.setLoopCount(QSoundEffect::Infinite);
        // the window turning an alarm off, or resetting a timer, silences its ring
        m_watch.addPaths({ClockStore::alarmsPath(), ClockStore::timersPath()});
        connect(&m_watch, &QFileSystemWatcher::fileChanged, this, &Ringer::filesChanged);
    }

    void start()
    {
        if (QDBusConnection::sessionBus().interface()->isServiceRegistered(kService)) {
            showDue();
            return;
        }
        // at sign-in the timer may come before the notification server
        auto *watcher = new QDBusServiceWatcher(kService, QDBusConnection::sessionBus(),
                                                QDBusServiceWatcher::WatchForRegistration, this);
        connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &Ringer::showDue);
        QTimer::singleShot(120000, this, [this] {
            if (m_shown.isEmpty())
                finish();
        });
    }

public Q_SLOTS:
    void showDue()
    {
        const QDateTime now = QDateTime::currentDateTime();
        const QTimeZone zone = QTimeZone::systemTimeZone();
        QList<Alarm> alarms = loadAlarms();
        QList<Countdown> timers = loadTimers();
        RingState state = loadState();
        bool alarmsChanged = false;
        bool timersChanged = false;
        QString sound;

        for (const Ring &r : Rings::due(alarms, timers, state, now, zone)) {
            if (r.kind == Ring::Timer) {
                auto it = std::find_if(timers.begin(), timers.end(), [&r](const Countdown &c) { return c.id == r.id; });
                const Countdown timer = *it;
                // over: it stands back at its full length, as on Windows
                it->reset();
                timersChanged = true;
                const uint id = notify(timer.name.isEmpty() ? i18nc("@title", "Timer") : timer.name,
                                       r.late ? i18nc("@info", "Timer ended at %1", timeText(r.at))
                                              : i18nc("@info timer length", "Time's up: %1", lengthText(timer.durationMs)),
                                       r.late ? QStringList()
                                              : QStringList{QStringLiteral("restart"), i18nc("@action:button", "Restart"),
                                                            QStringLiteral("dismiss"), i18nc("@action:button", "Dismiss")},
                                       !r.late);
                if (id && !r.late) {
                    m_shown.insert(id, r);
                    sound = timerSound();
                }
                continue;
            }

            auto it = std::find_if(alarms.begin(), alarms.end(), [&r](const Alarm &a) { return a.id == r.id; });
            const Alarm alarm = *it;
            if (r.kind == Ring::Snooze)
                state.snoozed.remove(r.id);
            else
                state.handled.insert(r.id, r.at.toUTC());
            const QString title = alarm.name.isEmpty() ? i18nc("@title", "Alarm") : alarm.name;
            if (r.late) {
                notify(i18nc("@title", "Missed alarm"), i18nc("@info alarm name, time", "%1, %2", title, timeText(r.at)), {}, false);
                if (!alarm.repeats()) {
                    it->enabled = false;
                    alarmsChanged = true;
                }
                continue;
            }
            QStringList actions;
            if (alarm.snoozeMinutes > 0)
                actions << QStringLiteral("snooze")
                        << i18ncp("@action:button", "Snooze %1 min", "Snooze %1 min", alarm.snoozeMinutes);
            actions << QStringLiteral("dismiss") << i18nc("@action:button", "Dismiss");
            const uint id = notify(title, timeText(QDateTime::currentDateTime()), actions, true);
            if (id) {
                m_shown.insert(id, r);
                sound = alarm.sound;
            }
        }

        if (timersChanged)
            ClockStore::writeAtomic(ClockStore::timersPath(), Countdowns::toJson(timers));
        if (alarmsChanged)
            ClockStore::writeAtomic(ClockStore::alarmsPath(), Alarms::toJson(alarms));
        ClockStore::writeAtomic(ClockStore::statePath(), state.toJson());

        if (!sound.isEmpty())
            play(sound);
        if (m_shown.isEmpty()) {
            finish();
            return;
        }
        rearm();
    }

    void actionInvoked(uint id, const QString &action)
    {
        const auto it = m_shown.constFind(id);
        if (it == m_shown.cend())
            return;
        const Ring ring = it.value();
        const QDateTime now = QDateTime::currentDateTime();
        if (action == QLatin1String("snooze") && ring.kind != Ring::Timer) {
            const Alarm alarm = alarmOf(ring.id);
            RingState state = loadState();
            state.snoozed.insert(ring.id, now.toUTC().addSecs(qint64(qMax(1, alarm.snoozeMinutes)) * 60));
            ClockStore::writeAtomic(ClockStore::statePath(), state.toJson());
            m_snoozed.insert(id);
        } else if (action == QLatin1String("restart") && ring.kind == Ring::Timer) {
            QList<Countdown> timers = loadTimers();
            for (Countdown &c : timers) {
                if (c.id == ring.id)
                    c.restart(now);
            }
            ClockStore::writeAtomic(ClockStore::timersPath(), Countdowns::toJson(timers));
        } else if (action == QLatin1String("default")) {
            // a unit of its own, so the window outlives this process
            QProcess::startDetached(QStringLiteral("systemd-run"),
                                    {QStringLiteral("--user"), QStringLiteral("--quiet"), QStringLiteral("--collect"),
                                     QStringLiteral(RAYE_CLOCK_APP),
                                     ring.kind == Ring::Timer ? QStringLiteral("--timers") : QStringLiteral("--alarms")});
        }
        QDBusInterface(kService, kPath, kService).call(QStringLiteral("CloseNotification"), id);
    }

    void closed(uint id, uint)
    {
        const auto it = m_shown.constFind(id);
        if (it == m_shown.cend())
            return;
        const Ring ring = it.value();
        m_shown.erase(it);
        // a one-time alarm answered with anything but Snooze is done
        if (ring.kind != Ring::Timer && !m_snoozed.remove(id)) {
            QList<Alarm> alarms = loadAlarms();
            for (Alarm &a : alarms) {
                if (a.id == ring.id && !a.repeats() && a.enabled) {
                    a.enabled = false;
                    ClockStore::writeAtomic(ClockStore::alarmsPath(), Alarms::toJson(alarms));
                    break;
                }
            }
        }
        if (m_shown.isEmpty()) {
            m_sound.stop();
            finish();
        } else {
            rearm();
        }
    }

private:
    uint notify(const QString &title, const QString &body, const QStringList &actions, bool urgent)
    {
        QStringList all{QStringLiteral("default"), i18nc("@action:button", "Open")};
        all += actions;
        QVariantMap hints{
            {QStringLiteral("desktop-entry"), QStringLiteral("org.raye.clock")},
            {QStringLiteral("category"), QStringLiteral("x-raye.clock.alarm")},
            // the clock plays its own sound, in a loop
            {QStringLiteral("suppress-sound"), true},
        };
        if (urgent)
            hints.insert(QStringLiteral("urgency"), QVariant::fromValue(uchar(2)));
        QDBusInterface n(kService, kPath, kService);
        // 0: on screen until answered
        const QDBusReply<uint> id = n.call(QStringLiteral("Notify"), i18nc("@title", "Clock"), 0u, QStringLiteral("org.raye.clock"),
                                           title, body, all, hints, urgent ? 0 : -1);
        return id.isValid() ? id.value() : 0;
    }

    void play(const QString &name)
    {
        const QString file = soundFile(name);
        if (file.isEmpty())
            return;
        m_sound.stop();
        m_sound.setSource(QUrl::fromLocalFile(file));
        m_sound.setVolume(0.25);
        m_sound.play();
        m_ramp.start();
        m_quiet.start(kRingMaxMs);
    }

    static Alarm alarmOf(const QString &id)
    {
        for (const Alarm &a : loadAlarms()) {
            if (a.id == id)
                return a;
        }
        return {};
    }

    void filesChanged(const QString &path)
    {
        // a rename replaces the file: watch the new one
        if (!m_watch.files().contains(path))
            m_watch.addPath(path);
        const QList<Alarm> alarms = loadAlarms();
        const QList<Countdown> timers = loadTimers();
        for (auto it = m_shown.cbegin(); it != m_shown.cend(); ++it) {
            const Ring &r = it.value();
            bool still;
            if (r.kind == Ring::Timer) {
                // reset by the ring itself; gone or running again means the window took it
                still = std::any_of(timers.cbegin(), timers.cend(), [&r](const Countdown &c) { return c.id == r.id && !c.running(); });
            } else {
                still = std::any_of(alarms.cbegin(), alarms.cend(), [&r](const Alarm &a) { return a.id == r.id && a.enabled; });
            }
            if (!still)
                QDBusInterface(kService, kPath, kService).call(QStringLiteral("CloseNotification"), it.key());
        }
        rearm();
    }

    // the timer for the one after, and a timer of our own while we are up:
    // systemd does not start a service that is already running
    void rearm()
    {
        const QDateTime now = QDateTime::currentDateTime();
        const QTimeZone zone = QTimeZone::systemTimeZone();
        const QList<Alarm> alarms = loadAlarms();
        const QList<Countdown> timers = loadTimers();
        const RingState state = loadState();
        Schedule::apply(alarms, timers, state, now, zone, ClockStore::timerPath(), Schedule::realSystemctl());
        const QDateTime next = Rings::next(alarms, timers, state, now, zone);
        if (next.isValid() && !m_shown.isEmpty())
            m_next.start(int(qBound(qint64(500), now.msecsTo(next), qint64(6) * 3600 * 1000)));
        else
            m_next.stop();
    }

    // nothing left on screen: one last look (a snooze may have come due),
    // the timer for the next one, and out
    void finish()
    {
        const QDateTime now = QDateTime::currentDateTime();
        const QTimeZone zone = QTimeZone::systemTimeZone();
        const QList<Alarm> alarms = loadAlarms();
        const QList<Countdown> timers = loadTimers();
        const RingState state = loadState();
        if (++m_rounds < 5 && !Rings::due(alarms, timers, state, now, zone).isEmpty()) {
            showDue();
            return;
        }
        // a ring a few seconds away: wait for it rather than come back
        const QDateTime next = Rings::next(alarms, timers, state, now, zone);
        if (m_rounds < 5 && next.isValid() && now.msecsTo(next) < 5000) {
            m_next.start(int(qMax(qint64(200), now.msecsTo(next))));
            return;
        }
        m_next.stop();
        m_sound.stop();
        Schedule::apply(alarms, timers, state, now, zone, ClockStore::timerPath(), Schedule::realSystemctl());
        // let the sound end cleanly
        QTimer::singleShot(200, qApp, &QCoreApplication::quit);
    }

    QHash<uint, Ring> m_shown;
    QSet<uint> m_snoozed;
    QSoundEffect m_sound;
    QTimer m_next;
    QTimer m_quiet;
    QTimer m_ramp;
    QFileSystemWatcher m_watch;
    int m_rounds = 0;
};

int schedule()
{
    const QList<Alarm> alarms = loadAlarms();
    const QList<Countdown> timers = loadTimers();
    RingState state = loadState();
    const auto action = Schedule::apply(alarms, timers, state, QDateTime::currentDateTime(), QTimeZone::systemTimeZone(),
                                        ClockStore::timerPath(), Schedule::realSystemctl());
    std::printf("%s\n", action == Schedule::Action::StartNow ? "due now" : action == Schedule::Action::Timer ? "timer" : "none");
    return 0;
}
} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("raye-clock"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral(TRANSLATION_DOMAIN));
    const QStringList args = app.arguments().mid(1);
    const QString command = args.value(0);
    if (command == QLatin1String("schedule"))
        return schedule();
    if (command == QLatin1String("ring")) {
        Ringer ringer;
        ringer.start();
        return QCoreApplication::exec();
    }
    std::fprintf(stderr, "usage: raye-clock-tool schedule | ring\n");
    return 2;
}

#include "tool.moc"
