// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// raye-calendar-tool: the calendar's part that runs without its window.
//
//   schedule          arms the user timer for the next reminder (or starts
//                     the reminder service when one is already due)
//   fire              what that timer starts: shows the reminders that are
//                     due, with Later and Dismiss, and arms the next one.
//                     It ends when the last reminder on screen is closed.
//   agenda FROM TO    the events of those days, as JSON, for the clock's
//                     calendar on the taskbar (dates as yyyy-MM-dd)

#include "calstore.h"
#include "holidaynames.h"
#include "holidays.h"

#include <KLocalizedString>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>
#include <QTimer>

#include <cstdio>

namespace
{
const QString kService = QStringLiteral("org.freedesktop.Notifications");
const QString kPath = QStringLiteral("/org/freedesktop/Notifications");
constexpr int kLaterMinutes = 5;

QString holidayRegion()
{
    QSettings s(CalStore::settingsPath(), QSettings::IniFormat);
    return s.value(QStringLiteral("Calendar/holidays")).toString();
}

QString timeText(const QDateTime &t)
{
    return QLocale().toString(t.toLocalTime().time(), QLocale::ShortFormat);
}

// "In 15 minutes", "Now", "Tomorrow, 9:00"...: when, said the short way
QString whenText(const CalEvent &ev, const Occurrence &o, const QDateTime &now)
{
    const QDate today = now.toLocalTime().date();
    const QDate day = o.start.toLocalTime().date();
    QString dayText = day == today ? i18nc("@info", "Today")
        : day == today.addDays(1)  ? i18nc("@info", "Tomorrow")
                                   : QLocale().toString(day, i18nc("@info date format, QDate pattern", "dddd, MMMM d"));
    if (ev.allDay)
        return dayText;
    const qint64 mins = (now.secsTo(o.start) + 59) / 60;
    QString lead;
    if (mins > 0 && mins < 60)
        lead = i18ncp("@info", "In %1 minute", "In %1 minutes", mins);
    else if (mins <= 0 && o.end > now)
        lead = i18nc("@info the event has begun", "Now");
    const QString range = i18nc("@info day, start - end", "%1, %2 – %3", dayText, timeText(o.start), timeText(o.end));
    return lead.isEmpty() ? range : lead + QLatin1Char('\n') + range;
}

class Fire : public QObject
{
    Q_OBJECT

public:
    Fire()
    {
        auto bus = QDBusConnection::sessionBus();
        bus.connect(kService, kPath, kService, QStringLiteral("ActionInvoked"), this, SLOT(actionInvoked(uint, QString)));
        bus.connect(kService, kPath, kService, QStringLiteral("NotificationClosed"), this, SLOT(closed(uint, uint)));
        m_next.setSingleShot(true);
        connect(&m_next, &QTimer::timeout, this, &Fire::showDue);
    }

    // waits for the notification server: at sign-in the timer may run first
    void start()
    {
        if (QDBusConnection::sessionBus().interface()->isServiceRegistered(kService)) {
            showDue();
            return;
        }
        auto *watcher = new QDBusServiceWatcher(kService, QDBusConnection::sessionBus(),
                                                QDBusServiceWatcher::WatchForRegistration, this);
        connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &Fire::showDue);
        QTimer::singleShot(120000, this, [this] {
            if (m_shown.isEmpty())
                finish();
        });
    }

public Q_SLOTS:
    void showDue()
    {
        const QDateTime now = QDateTime::currentDateTime();
        m_events = CalStore::loadOrEmpty(CalStore::calendarPath()).events;
        ReminderState state = CalStore::loadState(CalStore::statePath());
        QDBusInterface n(kService, kPath, kService);
        for (const Reminder &r : Reminders::due(m_events, state, now)) {
            const CalEvent &ev = m_events[r.event];
            QString body = whenText(ev, r.occurrence, now);
            if (!ev.location.isEmpty())
                body += QLatin1Char('\n') + ev.location;
            const QStringList actions{
                QStringLiteral("default"), i18nc("@action:button", "Open"),
                QStringLiteral("later"), i18nc("@action:button remind again in a few minutes", "Later"),
                QStringLiteral("dismiss"), i18nc("@action:button", "Dismiss"),
            };
            const QVariantMap hints{
                {QStringLiteral("desktop-entry"), QStringLiteral("org.raye.calendar")},
                {QStringLiteral("category"), QStringLiteral("x-raye.calendar.reminder")},
            };
            const QString title = ev.summary.isEmpty() ? i18nc("@info", "Event") : ev.summary;
            // 0: on screen until answered, like a reminder should
            const QDBusReply<uint> id = n.call(QStringLiteral("Notify"), i18nc("@title", "Calendar"), 0u,
                                               QStringLiteral("org.raye.calendar"), title, body, actions, hints, 0);
            if (!id.isValid())
                continue;
            m_shown.insert(id.value(), {r.key, r.occurrence.start, ev.uid});
            state.done.insert(r.key, now);
            state.snoozed.remove(r.key);
        }
        state.prune(now);
        CalStore::saveState(CalStore::statePath(), state);
        if (m_shown.isEmpty()) {
            finish();
            return;
        }
        rearm(state, now);
    }

    void actionInvoked(uint id, const QString &action)
    {
        const auto it = m_shown.constFind(id);
        if (it == m_shown.cend())
            return;
        const Shown shown = it.value();
        if (action == QLatin1String("later")) {
            const QDateTime now = QDateTime::currentDateTime();
            ReminderState state = CalStore::loadState(CalStore::statePath());
            // well before the start: back five minutes before it, not in five minutes
            QDateTime until = now.addSecs(kLaterMinutes * 60);
            if (shown.start.addSecs(-kLaterMinutes * 60) > until.addSecs(10 * 60))
                until = shown.start.addSecs(-kLaterMinutes * 60);
            state.snoozed.insert(shown.key, until);
            CalStore::saveState(CalStore::statePath(), state);
            rearm(state, now);
        } else if (action == QLatin1String("default")) {
            // a unit of its own, so the window outlives this process
            QProcess::startDetached(QStringLiteral("systemd-run"),
                                    {QStringLiteral("--user"), QStringLiteral("--quiet"), QStringLiteral("--collect"),
                                     QStringLiteral(RAYE_CALENDAR_APP),
                                     QStringLiteral("raye-calendar:?day=") + shown.start.toLocalTime().date().toString(Qt::ISODate)});
        }
        QDBusInterface(kService, kPath, kService).call(QStringLiteral("CloseNotification"), id);
    }

    void closed(uint id, uint)
    {
        if (m_shown.remove(id) && m_shown.isEmpty())
            finish();
    }

private:
    struct Shown
    {
        QString key;
        QDateTime start;
        QString uid;
    };

    // the timer for the one after, and a timer of our own while we are up:
    // systemd does not start a service that is already running
    void rearm(const ReminderState &state, const QDateTime &now)
    {
        Schedule::apply(m_events, state, now, CalStore::timerPath(), Schedule::realSystemctl());
        const qint64 wait = Reminders::wakeWhileShownMs(now, Reminders::next(m_events, state, now));
        if (wait > 0 && !m_shown.isEmpty())
            m_next.start(int(wait));
        else
            m_next.stop();
    }

    // nothing left on screen: one last look (a Later may have come due),
    // the timer for the next one, and out
    void finish()
    {
        const QDateTime now = QDateTime::currentDateTime();
        const ReminderState state = CalStore::loadState(CalStore::statePath());
        if (++m_rounds < 5 && !Reminders::due(m_events, state, now).isEmpty()) {
            showDue();
            return;
        }
        m_next.stop();
        Schedule::apply(m_events, state, now, CalStore::timerPath(), Schedule::realSystemctl());
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
    }

    QList<CalEvent> m_events;
    QHash<uint, Shown> m_shown;
    QTimer m_next;
    int m_rounds = 0;
};

int schedule()
{
    const QList<CalEvent> events = CalStore::loadOrEmpty(CalStore::calendarPath()).events;
    const ReminderState state = CalStore::loadState(CalStore::statePath());
    const auto action = Schedule::apply(events, state, QDateTime::currentDateTime(), CalStore::timerPath(),
                                        Schedule::realSystemctl());
    std::printf("%s\n", action == Schedule::Action::StartNow ? "due now" : action == Schedule::Action::Timer ? "timer" : "none");
    return 0;
}

int agenda(const QString &fromText, const QString &toText)
{
    const QDate from = QDate::fromString(fromText, Qt::ISODate);
    const QDate to = QDate::fromString(toText, Qt::ISODate);
    if (!from.isValid() || !to.isValid() || to < from || from.daysTo(to) > 400) {
        std::fprintf(stderr, "agenda: give two dates, yyyy-MM-dd, the second after the first\n");
        return 2;
    }
    const QList<CalEvent> events = CalStore::loadOrEmpty(CalStore::calendarPath()).events;
    QJsonArray out;
    const QDateTime start(from, QTime(0, 0));
    const QDateTime end(to.addDays(1), QTime(0, 0));
    for (const CalEvent &ev : events) {
        for (const Occurrence &o : Recurrence::between(ev, start, end)) {
            out.append(QJsonObject{
                {QStringLiteral("title"), ev.summary},
                {QStringLiteral("start"), o.start.toLocalTime().toString(Qt::ISODate)},
                {QStringLiteral("end"), o.end.toLocalTime().toString(Qt::ISODate)},
                {QStringLiteral("allDay"), ev.allDay},
                {QStringLiteral("color"), ev.color},
                {QStringLiteral("location"), ev.location},
            });
        }
    }
    for (const Holiday &h : Holidays::between(holidayRegion(), from, to)) {
        out.append(QJsonObject{
            {QStringLiteral("title"), holidayName(h.id)},
            {QStringLiteral("start"), QDateTime(h.date, QTime(0, 0)).toString(Qt::ISODate)},
            {QStringLiteral("end"), QDateTime(h.date.addDays(1), QTime(0, 0)).toString(Qt::ISODate)},
            {QStringLiteral("allDay"), true},
            {QStringLiteral("holiday"), true},
        });
    }
    std::fwrite(QJsonDocument(out).toJson(QJsonDocument::Compact).constData(), 1,
                QJsonDocument(out).toJson(QJsonDocument::Compact).size(), stdout);
    std::printf("\n");
    return 0;
}
} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("raye-calendar"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral(TRANSLATION_DOMAIN));
    const QStringList args = app.arguments().mid(1);
    const QString command = args.value(0);
    if (command == QLatin1String("schedule"))
        return schedule();
    if (command == QLatin1String("agenda") && args.size() == 3)
        return agenda(args[1], args[2]);
    if (command == QLatin1String("fire")) {
        Fire fire;
        fire.start();
        return QCoreApplication::exec();
    }
    std::fprintf(stderr, "usage: raye-calendar-tool schedule | fire | agenda FROM TO\n");
    return 2;
}

#include "tool.moc"
