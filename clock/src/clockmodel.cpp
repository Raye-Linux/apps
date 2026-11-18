// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "clockmodel.h"
#include "cities.h"
#include "clockstore.h"
#include "worldtime.h"

#include <KLocalizedString>

#include <QCollator>
#include <QFile>
#include <QLocale>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>

namespace
{
const QStringList kAlarmRoles{QStringLiteral("key"), QStringLiteral("name"), QStringLiteral("hour"),
                              QStringLiteral("minute"), QStringLiteral("clock"), QStringLiteral("meridiem"),
                              QStringLiteral("days"), QStringLiteral("enabled"), QStringLiteral("sound"),
                              QStringLiteral("snooze"), QStringLiteral("ringsIn"), QStringLiteral("snoozed")};
const QStringList kTimerRoles{QStringLiteral("key"), QStringLiteral("name"), QStringLiteral("durationMs"),
                              QStringLiteral("leftMs"), QStringLiteral("endsAt"), QStringLiteral("running")};
const QStringList kCityRoles{QStringLiteral("key"), QStringLiteral("name"), QStringLiteral("country"),
                             QStringLiteral("clock"), QStringLiteral("meridiem"), QStringLiteral("dayText"),
                             QStringLiteral("gapText"), QStringLiteral("daytime"), QStringLiteral("gapMinutes")};
const QStringList kSoundIds{QStringLiteral("glow"), QStringLiteral("chimes"), QStringLiteral("marimba"),
                            QStringLiteral("pulse"), QStringLiteral("dawn")};

QString soundName(const QString &id)
{
    if (id == QLatin1String("chimes"))
        return i18nc("@item alarm sound", "Chimes");
    if (id == QLatin1String("marimba"))
        return i18nc("@item alarm sound", "Marimba");
    if (id == QLatin1String("pulse"))
        return i18nc("@item alarm sound", "Pulse");
    if (id == QLatin1String("dawn"))
        return i18nc("@item alarm sound", "Dawn");
    return i18nc("@item alarm sound", "Glow");
}

} // namespace

ClockModel::ClockModel(QObject *parent)
    : QObject(parent)
    , m_alarmRows(kAlarmRoles)
    , m_timerRows(kTimerRoles)
    , m_cityRows(kCityRoles)
{
    // the ring tool writes through a rename: a short wait, then read it all
    m_reload.setSingleShot(true);
    m_reload.setInterval(150);
    connect(&m_reload, &QTimer::timeout, this, &ClockModel::load);
    connect(&m_watch, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        if (!m_watch.files().contains(path))
            m_watch.addPath(path);
        m_reload.start();
    });
    connect(&m_watch, &QFileSystemWatcher::directoryChanged, &m_reload, qOverload<>(&QTimer::start));

    m_minute.setSingleShot(true);
    connect(&m_minute, &QTimer::timeout, this, [this] {
        // the computer moved to another zone: the next ring is somewhere else
        if (QTimeZone::systemTimeZoneId() != m_zoneId) {
            m_zoneId = QTimeZone::systemTimeZoneId();
            schedule();
        }
        refreshAlarms();
        refreshCities();
        Q_EMIT minuteChanged();
        armMinute();
    });

    // a new user starts with four timers, as on Windows
    if (!QFile::exists(ClockStore::timersPath()))
        ClockStore::writeAtomic(ClockStore::timersPath(), Countdowns::toJson(Countdowns::starters()));
    for (const QString &path : {ClockStore::alarmsPath(), ClockStore::timersPath(), ClockStore::statePath()}) {
        if (QFile::exists(path))
            m_watch.addPath(path);
    }
    for (const QString &path : {ClockStore::alarmsPath(), ClockStore::statePath()}) {
        const QString dir = path.left(path.lastIndexOf(QLatin1Char('/')));
        if (QFile::exists(dir))
            m_watch.addPath(dir);
    }
    load();
    loadStopwatch();
    refreshCities();
    armMinute();
    m_zoneId = QTimeZone::systemTimeZoneId();
    // a zone or a clock that changed while the window was closed
    schedule();
}

void ClockModel::armMinute()
{
    const QTime t = QTime::currentTime();
    m_minute.start((60 - t.second()) * 1000 - t.msec() + 50);
}

void ClockModel::load()
{
    m_alarmList = Alarms::fromJson(ClockStore::read(ClockStore::alarmsPath()));
    m_timerList = Countdowns::fromJson(ClockStore::read(ClockStore::timersPath()));
    m_state = RingState::fromJson(ClockStore::read(ClockStore::statePath()));
    refreshAlarms();
    refreshTimers();
}

void ClockModel::loadStopwatch()
{
    m_stopwatch = Stopwatch::fromJson(ClockStore::read(ClockStore::stopwatchPath()));
    Q_EMIT stopwatchChanged();
}

bool ClockModel::use12Hour() const
{
    return QLocale().timeFormat(QLocale::ShortFormat).contains(QLatin1Char('a'), Qt::CaseInsensitive);
}

QString ClockModel::localZone() const
{
    return QString::fromUtf8(QTimeZone::systemTimeZoneId());
}

namespace
{
// "7:00" and "AM", or "07:00" and nothing
QPair<QString, QString> clockParts(const QTime &t, bool twelve, bool seconds = false)
{
    const QLocale l;
    const QString sec = seconds ? QStringLiteral(":%1").arg(t.second(), 2, 10, QLatin1Char('0')) : QString();
    if (!twelve)
        return {QStringLiteral("%1:%2").arg(t.hour(), 2, 10, QLatin1Char('0')).arg(t.minute(), 2, 10, QLatin1Char('0')) + sec, {}};
    // "h" alone is the 24-hour hour in Qt: it only counts to 12 next to "AP"
    const int h = t.hour() % 12 == 0 ? 12 : t.hour() % 12;
    return {QStringLiteral("%1:%2").arg(h).arg(t.minute(), 2, 10, QLatin1Char('0')) + sec,
            t.hour() < 12 ? l.amText() : l.pmText()};
}
} // namespace

QVariantMap ClockModel::timeParts(const QDateTime &t, bool seconds) const
{
    const auto [clock, meridiem] = clockParts(t.toLocalTime().time(), use12Hour(), seconds);
    return {{QStringLiteral("clock"), clock}, {QStringLiteral("meridiem"), meridiem}};
}

QString ClockModel::localPlace() const
{
    const City c = Cities::of(localZone());
    if (c.zone.isEmpty())
        return localZone();
    return c.country.isEmpty() ? c.name : i18nc("@info city, country", "%1, %2", c.name, c.country);
}

QString ClockModel::ringsInText(const QDateTime &at) const
{
    if (!at.isValid())
        return {};
    const Duration::Wait w = Duration::until(QDateTime::currentDateTime(), at);
    QStringList parts;
    if (w.days)
        parts << i18ncp("@info part of: Rings in…", "%1 day", "%1 days", w.days);
    if (w.hours)
        parts << i18ncp("@info part of: Rings in…", "%1 hour", "%1 hours", w.hours);
    // with days, the minutes add nothing
    if (w.minutes && !w.days)
        parts << i18ncp("@info part of: Rings in…", "%1 minute", "%1 minutes", w.minutes);
    if (parts.isEmpty())
        return i18nc("@info", "Rings in less than a minute");
    if (parts.size() == 1)
        return i18nc("@info %1 is a length of time", "Rings in %1", parts[0]);
    return i18nc("@info %1 and %2 are lengths of time", "Rings in %1, %2", parts[0], parts[1]);
}

void ClockModel::refreshAlarms()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QTimeZone zone = QTimeZone::systemTimeZone();
    const bool twelve = use12Hour();
    QList<QVariantMap> rows;
    for (const Alarm &a : m_alarmList) {
        const auto [clock, meridiem] = clockParts(a.time, twelve);
        const QDateTime snooze = m_state.snoozed.value(a.id);
        const bool snoozed = a.enabled && snooze.isValid() && snooze > now;
        rows << QVariantMap{
            {QStringLiteral("key"), a.id},
            {QStringLiteral("name"), a.name},
            {QStringLiteral("hour"), a.time.hour()},
            {QStringLiteral("minute"), a.time.minute()},
            {QStringLiteral("clock"), clock},
            {QStringLiteral("meridiem"), meridiem},
            {QStringLiteral("days"), a.days},
            {QStringLiteral("enabled"), a.enabled},
            {QStringLiteral("sound"), a.sound},
            {QStringLiteral("snooze"), a.snoozeMinutes},
            {QStringLiteral("ringsIn"), ringsInText(Rings::nextOf(a, m_state, now, zone))},
            {QStringLiteral("snoozed"), snoozed ? i18nc("@info", "Snoozed until %1", QLocale().toString(snooze.toLocalTime().time(), QLocale::ShortFormat)) : QString()},
        };
    }
    m_alarmRows.setRows(rows);
}

void ClockModel::refreshTimers()
{
    QList<QVariantMap> rows;
    for (const Countdown &c : m_timerList) {
        rows << QVariantMap{
            {QStringLiteral("key"), c.id},
            {QStringLiteral("name"), c.name},
            {QStringLiteral("durationMs"), double(c.durationMs)},
            {QStringLiteral("leftMs"), c.running() ? 0.0 : double(c.remainingMs(QDateTime()))},
            {QStringLiteral("endsAt"), c.running() ? double(c.endsAt.toMSecsSinceEpoch()) : 0.0},
            {QStringLiteral("running"), c.running()},
        };
    }
    m_timerRows.setRows(rows);
    const bool running = timersRunning();
    if (running != m_timersRunning) {
        m_timersRunning = running;
        Q_EMIT timersRunningChanged();
    }
}

bool ClockModel::timersRunning() const
{
    return std::any_of(m_timerList.cbegin(), m_timerList.cend(), [](const Countdown &c) { return c.running(); });
}

QStringList ClockModel::cityKeys() const
{
    QSettings s(ClockStore::settingsPath(), QSettings::IniFormat);
    return s.value(QStringLiteral("WorldClock/cities")).toStringList();
}

void ClockModel::refreshCities()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QTimeZone here = QTimeZone::systemTimeZone();
    const bool twelve = use12Hour();
    QList<QVariantMap> rows;
    for (const QString &key : cityKeys()) {
        const City city = Cities::of(key);
        if (city.zone.isEmpty())
            continue;
        const QTimeZone zone(city.zone);
        const WorldTime::Gap gap = WorldTime::compare(zone, here, now);
        const auto [clock, meridiem] = clockParts(now.toTimeZone(zone).time(), twelve);
        const QString dayText = gap.days > 0 ? i18nc("@info", "Tomorrow")
                              : gap.days < 0 ? i18nc("@info", "Yesterday")
                                             : i18nc("@info", "Today");
        const int h = std::abs(gap.minutes) / 60;
        const int m = std::abs(gap.minutes) % 60;
        QString gapText;
        if (gap.minutes == 0)
            gapText = i18nc("@info", "Same time");
        else if (m == 0)
            gapText = gap.minutes > 0 ? i18ncp("@info", "%1 hour ahead", "%1 hours ahead", h)
                                      : i18ncp("@info", "%1 hour behind", "%1 hours behind", h);
        else
            gapText = gap.minutes > 0 ? i18nc("@info hours and minutes", "%1 h %2 min ahead", h, m)
                                      : i18nc("@info hours and minutes", "%1 h %2 min behind", h, m);
        rows << QVariantMap{
            {QStringLiteral("key"), key},
            {QStringLiteral("name"), city.name},
            {QStringLiteral("country"), city.country},
            {QStringLiteral("clock"), clock},
            {QStringLiteral("meridiem"), meridiem},
            {QStringLiteral("dayText"), dayText},
            {QStringLiteral("gapText"), gapText},
            {QStringLiteral("daytime"), WorldTime::daytime(zone, now)},
            {QStringLiteral("gapMinutes"), gap.minutes},
        };
    }
    m_cityRows.setRows(rows);
}

QVariantList ClockModel::laps() const
{
    QVariantList out;
    for (const Stopwatch::Row &r : m_stopwatch.rows()) {
        out << QVariantMap{
            {QStringLiteral("number"), r.number},
            {QStringLiteral("lap"), Duration::preciseText(r.lapMs)},
            {QStringLiteral("total"), Duration::preciseText(r.totalMs)},
            {QStringLiteral("fastest"), r.fastest},
            {QStringLiteral("slowest"), r.slowest},
        };
    }
    return out;
}

QVariantList ClockModel::sounds() const
{
    QVariantList out;
    for (const QString &id : kSoundIds)
        out << QVariantMap{{QStringLiteral("value"), id}, {QStringLiteral("text"), soundName(id)}};
    return out;
}

QUrl ClockModel::soundUrl(const QString &id) const
{
    const QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                QStringLiteral("raye-clock/sounds/%1.wav").arg(id));
    return path.isEmpty() ? QUrl() : QUrl::fromLocalFile(path);
}

QString ClockModel::timerSound() const
{
    QSettings s(ClockStore::settingsPath(), QSettings::IniFormat);
    return s.value(QStringLiteral("Timers/sound"), QStringLiteral("chimes")).toString();
}

void ClockModel::setTimerSound(const QString &id)
{
    if (!kSoundIds.contains(id) || id == timerSound())
        return;
    QSettings s(ClockStore::settingsPath(), QSettings::IniFormat);
    s.setValue(QStringLiteral("Timers/sound"), id);
    Q_EMIT timerSoundChanged();
}

QString ClockModel::dayLetter(int dayOfWeek) const
{
    // two letters, capitalized: "Mo", "Lu"
    QString s = QLocale().standaloneDayName(dayOfWeek, QLocale::ShortFormat).left(2);
    if (!s.isEmpty())
        s[0] = s[0].toUpper();
    return s;
}

QString ClockModel::dayName(int dayOfWeek) const
{
    return QLocale().standaloneDayName(dayOfWeek, QLocale::LongFormat);
}

void ClockModel::setError(const QString &error)
{
    if (m_error == error)
        return;
    m_error = error;
    Q_EMIT lastErrorChanged();
}

void ClockModel::writeAlarms()
{
    QString error;
    if (!ClockStore::writeAtomic(ClockStore::alarmsPath(), Alarms::toJson(m_alarmList), &error))
        setError(i18nc("@info", "Could not save the alarms: %1", error));
    else
        setError({});
    m_watch.addPath(ClockStore::alarmsPath());
}

void ClockModel::writeTimers()
{
    QString error;
    if (!ClockStore::writeAtomic(ClockStore::timersPath(), Countdowns::toJson(m_timerList), &error))
        setError(i18nc("@info", "Could not save the timers: %1", error));
    else
        setError({});
    m_watch.addPath(ClockStore::timersPath());
}

void ClockModel::writeState()
{
    m_state.keepOnly(m_alarmList);
    ClockStore::writeAtomic(ClockStore::statePath(), m_state.toJson());
    m_watch.addPath(ClockStore::statePath());
}

void ClockModel::writeStopwatch()
{
    ClockStore::writeAtomic(ClockStore::stopwatchPath(), m_stopwatch.toJson());
    Q_EMIT stopwatchChanged();
}

void ClockModel::schedule()
{
    QProcess::startDetached(QStringLiteral(RAYE_CLOCK_TOOL), {QStringLiteral("schedule")});
}

QString ClockModel::saveAlarm(const QString &key, const QVariantMap &fields)
{
    auto it = std::find_if(m_alarmList.begin(), m_alarmList.end(), [&key](const Alarm &a) { return a.id == key; });
    Alarm a = it != m_alarmList.end() ? *it : Alarm();
    if (a.id.isEmpty())
        a.id = Alarms::newId();
    a.name = fields.value(QStringLiteral("name")).toString().trimmed().left(80);
    a.time = QTime(qBound(0, fields.value(QStringLiteral("hour")).toInt(), 23), qBound(0, fields.value(QStringLiteral("minute")).toInt(), 59));
    a.days = fields.value(QStringLiteral("days")).toInt() & Alarms::kEveryDay;
    const QString sound = fields.value(QStringLiteral("sound")).toString();
    a.sound = kSoundIds.contains(sound) ? sound : kSoundIds.first();
    a.snoozeMinutes = qBound(0, fields.value(QStringLiteral("snooze"), 10).toInt(), 60);
    // saving an alarm switches it on, as on Windows
    a.enabled = true;
    a.armed = QDateTime::currentDateTimeUtc();
    m_state.snoozed.remove(a.id);
    if (it != m_alarmList.end())
        *it = a;
    else
        m_alarmList << a;
    writeAlarms();
    writeState();
    refreshAlarms();
    schedule();
    return a.id;
}

void ClockModel::removeAlarm(const QString &key)
{
    m_alarmList.removeIf([&key](const Alarm &a) { return a.id == key; });
    writeAlarms();
    writeState();
    refreshAlarms();
    schedule();
}

void ClockModel::setAlarmEnabled(const QString &key, bool on)
{
    for (Alarm &a : m_alarmList) {
        if (a.id != key || a.enabled == on)
            continue;
        a.enabled = on;
        if (on)
            a.armed = QDateTime::currentDateTimeUtc();
        m_state.snoozed.remove(a.id);
        writeAlarms();
        writeState();
        refreshAlarms();
        schedule();
        return;
    }
}

QString ClockModel::addTimer(const QString &name, double ms)
{
    Countdown c;
    c.id = Alarms::newId();
    c.name = name.trimmed().left(80);
    c.setDuration(qint64(ms));
    m_timerList << c;
    writeTimers();
    refreshTimers();
    return c.id;
}

void ClockModel::editTimer(const QString &key, const QString &name, double ms)
{
    for (Countdown &c : m_timerList) {
        if (c.id != key)
            continue;
        c.name = name.trimmed().left(80);
        if (qint64(ms) != c.durationMs)
            c.setDuration(qint64(ms));
    }
    writeTimers();
    refreshTimers();
    schedule();
}

void ClockModel::removeTimer(const QString &key)
{
    m_timerList.removeIf([&key](const Countdown &c) { return c.id == key; });
    writeTimers();
    refreshTimers();
    schedule();
}

void ClockModel::startTimer(const QString &key)
{
    for (Countdown &c : m_timerList) {
        if (c.id == key)
            c.start(QDateTime::currentDateTimeUtc());
    }
    writeTimers();
    refreshTimers();
    schedule();
}

void ClockModel::pauseTimer(const QString &key)
{
    for (Countdown &c : m_timerList) {
        if (c.id == key)
            c.pause(QDateTime::currentDateTimeUtc());
    }
    writeTimers();
    refreshTimers();
    schedule();
}

void ClockModel::resetTimer(const QString &key)
{
    for (Countdown &c : m_timerList) {
        if (c.id == key)
            c.reset();
    }
    writeTimers();
    refreshTimers();
    schedule();
}

void ClockModel::startStopwatch()
{
    m_stopwatch.start(QDateTime::currentDateTimeUtc());
    writeStopwatch();
}

void ClockModel::pauseStopwatch()
{
    m_stopwatch.pause(QDateTime::currentDateTimeUtc());
    writeStopwatch();
}

void ClockModel::lap()
{
    m_stopwatch.lap(QDateTime::currentDateTimeUtc());
    writeStopwatch();
}

void ClockModel::resetStopwatch()
{
    m_stopwatch.reset();
    writeStopwatch();
}

QVariantList ClockModel::searchCities(const QString &query) const
{
    const QStringList taken = cityKeys();
    QList<City> found;
    for (const City &c : Cities::all()) {
        if (taken.contains(c.key) || !WorldTime::matches(query, {c.name, c.country, QString::fromLatin1(c.zone)}))
            continue;
        // with nothing typed, only the cities named by hand
        if (query.trimmed().isEmpty() && !c.known)
            continue;
        found << c;
    }
    // the big cities first, each group in alphabetical order
    QCollator byName;
    byName.setCaseSensitivity(Qt::CaseInsensitive);
    std::stable_sort(found.begin(), found.end(), [&byName](const City &a, const City &b) {
        if (a.known != b.known)
            return a.known;
        return byName.compare(a.name, b.name) < 0;
    });
    QVariantList out;
    for (const City &c : found.mid(0, 60))
        out << QVariantMap{{QStringLiteral("key"), c.key}, {QStringLiteral("name"), c.name}, {QStringLiteral("country"), c.country}};
    return out;
}

void ClockModel::addCity(const QString &key)
{
    if (Cities::of(key).zone.isEmpty())
        return;
    QStringList keys = cityKeys();
    if (keys.contains(key))
        return;
    keys << key;
    QSettings s(ClockStore::settingsPath(), QSettings::IniFormat);
    s.setValue(QStringLiteral("WorldClock/cities"), keys);
    s.sync();
    refreshCities();
}

void ClockModel::removeCity(const QString &key)
{
    QStringList keys = cityKeys();
    keys.removeAll(key);
    QSettings s(ClockStore::settingsPath(), QSettings::IniFormat);
    s.setValue(QStringLiteral("WorldClock/cities"), keys);
    s.sync();
    refreshCities();
}
