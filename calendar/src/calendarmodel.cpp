// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "calendarmodel.h"

#include "calstore.h"
#include "holidaynames.h"
#include "holidays.h"
#include "recurrence.h"

#include <KLocalizedString>

#include <QFile>
#include <QProcess>
#include <QSettings>
#include <QTimeZone>

#include <algorithm>

namespace
{
// the colours an event can take, in the order the editor shows them;
// the first is "the accent", stored as nothing
const QStringList kColors{QString(), QStringLiteral("#2f80ed"), QStringLiteral("#1f9e75"), QStringLiteral("#d9822b"),
                          QStringLiteral("#d6455d"), QStringLiteral("#8a8396")};

Rule::Freq freqOf(const QString &s)
{
    return s == QLatin1String("daily") ? Rule::Daily
        : s == QLatin1String("weekly") ? Rule::Weekly
        : s == QLatin1String("monthly") ? Rule::Monthly
        : s == QLatin1String("yearly") ? Rule::Yearly
                                       : Rule::None;
}

QString freqName(Rule::Freq f)
{
    switch (f) {
    case Rule::Daily:
        return QStringLiteral("daily");
    case Rule::Weekly:
        return QStringLiteral("weekly");
    case Rule::Monthly:
        return QStringLiteral("monthly");
    case Rule::Yearly:
        return QStringLiteral("yearly");
    case Rule::None:
        break;
    }
    return QStringLiteral("none");
}
} // namespace

CalendarModel::CalendarModel(QObject *parent)
    : QObject(parent)
{
    QSettings s(CalStore::settingsPath(), QSettings::IniFormat);
    m_region = s.value(QStringLiteral("Calendar/holidays")).toString();
    reload();
}

void CalendarModel::reload()
{
    m_file = CalStore::loadOrEmpty(CalStore::calendarPath());
    ++m_revision;
}

bool CalendarModel::write()
{
    QString error;
    if (!CalStore::save(CalStore::calendarPath(), m_file, &error)) {
        m_error = i18nc("@info", "Your calendar could not be saved: %1", error);
        Q_EMIT changed();
        return false;
    }
    m_error.clear();
    ++m_revision;
    Q_EMIT changed();
    // the timer of the next reminder; quick, but not the window's business
    QProcess::startDetached(QStringLiteral(RAYE_CALENDAR_TOOL), {QStringLiteral("schedule")});
    return true;
}

int CalendarModel::indexOf(const QString &uid) const
{
    for (int i = 0; i < m_file.events.size(); ++i) {
        if (m_file.events[i].uid == uid)
            return i;
    }
    return -1;
}

void CalendarModel::setHolidayRegion(const QString &region)
{
    if (region == m_region)
        return;
    m_region = region;
    QSettings s(CalStore::settingsPath(), QSettings::IniFormat);
    s.setValue(QStringLiteral("Calendar/holidays"), region);
    ++m_revision;
    Q_EMIT changed();
}

QVariantList CalendarModel::holidayRegions() const
{
    QVariantList out;
    out << QVariantMap{{QStringLiteral("id"), QString()}, {QStringLiteral("name"), i18nc("@item no holidays", "None")}};
    const QString mine = Holidays::regionFor(QLocale());
    QStringList regions = Holidays::regions();
    // the user's own country first
    if (regions.removeOne(mine))
        regions.prepend(mine);
    for (const QString &r : std::as_const(regions))
        out << QVariantMap{{QStringLiteral("id"), r}, {QStringLiteral("name"), holidayRegionName(r)}};
    return out;
}

QVariantList CalendarModel::occurrences(const QDateTime &from, const QDateTime &to) const
{
    struct Row
    {
        QDateTime start;
        QVariantMap map;
    };
    QList<Row> rows;
    for (const CalEvent &ev : m_file.events) {
        for (const Occurrence &o : Recurrence::between(ev, from, to)) {
            rows << Row{o.start,
                        {{QStringLiteral("uid"), ev.uid},
                         {QStringLiteral("title"), ev.summary},
                         {QStringLiteral("start"), o.start.toLocalTime()},
                         {QStringLiteral("end"), o.end.toLocalTime()},
                         {QStringLiteral("allDay"), ev.allDay},
                         {QStringLiteral("color"), ev.color},
                         {QStringLiteral("location"), ev.location},
                         {QStringLiteral("repeats"), !ev.rule.isNull()},
                         {QStringLiteral("alarm"), ev.hasAlarm},
                         {QStringLiteral("holiday"), false}}};
        }
    }
    for (const Holiday &h : Holidays::between(m_region, from.date(), to.addSecs(-1).date())) {
        const QDateTime s(h.date, QTime(0, 0));
        rows << Row{s,
                    {{QStringLiteral("uid"), QString()},
                     {QStringLiteral("title"), holidayName(h.id)},
                     {QStringLiteral("start"), s},
                     {QStringLiteral("end"), s.addDays(1)},
                     {QStringLiteral("allDay"), true},
                     {QStringLiteral("color"), QString()},
                     {QStringLiteral("holiday"), true}}};
    }
    std::stable_sort(rows.begin(), rows.end(), [](const Row &a, const Row &b) {
        if (a.start != b.start)
            return a.start < b.start;
        return a.map.value(QStringLiteral("title")).toString() < b.map.value(QStringLiteral("title")).toString();
    });
    QVariantList out;
    for (const Row &r : rows)
        out << r.map;
    return out;
}

QVariantMap CalendarModel::event(const QString &uid) const
{
    const int i = indexOf(uid);
    if (i < 0)
        return {};
    const CalEvent &ev = m_file.events[i];
    const QDateTime start = ev.allDay ? QDateTime(ev.startDay, QTime(0, 0)) : ev.start.toLocalTime();
    // an all-day end is exclusive in the file and inclusive on screen
    const QDateTime end = ev.allDay ? QDateTime(ev.endDay.addDays(-1), QTime(0, 0)) : ev.end.toLocalTime();
    return {
        {QStringLiteral("uid"), ev.uid},
        {QStringLiteral("title"), ev.summary},
        {QStringLiteral("location"), ev.location},
        {QStringLiteral("notes"), ev.description},
        {QStringLiteral("allDay"), ev.allDay},
        {QStringLiteral("start"), start},
        {QStringLiteral("end"), end},
        {QStringLiteral("repeat"), freqName(ev.rule.freq)},
        {QStringLiteral("repeatUnknown"), ev.rule.unsupported},
        {QStringLiteral("hasAlarm"), ev.hasAlarm},
        {QStringLiteral("alarmMinutes"), ev.alarmMinutes},
        {QStringLiteral("color"), ev.color},
    };
}

QString CalendarModel::save(const QVariantMap &f)
{
    const QString uid = f.value(QStringLiteral("uid")).toString();
    const int i = indexOf(uid);
    CalEvent ev = i >= 0 ? m_file.events[i] : CalEvent{};
    if (i < 0)
        ev.uid = Ical::newUid();
    ev.summary = f.value(QStringLiteral("title")).toString().trimmed();
    ev.location = f.value(QStringLiteral("location")).toString().trimmed();
    ev.description = f.value(QStringLiteral("notes")).toString();
    ev.color = f.value(QStringLiteral("color")).toString();
    ev.allDay = f.value(QStringLiteral("allDay")).toBool();
    const QDateTime start = f.value(QStringLiteral("start")).toDateTime();
    QDateTime end = f.value(QStringLiteral("end")).toDateTime();
    if (ev.allDay) {
        ev.startDay = start.date();
        ev.endDay = qMax(end.date(), ev.startDay).addDays(1);
        ev.start = {};
        ev.end = {};
        ev.exdates.clear();
    } else {
        // in the zone of this computer, by its name, so a repeating 9:00
        // stays 9:00 when the hour changes
        const QTimeZone zone = QTimeZone::systemTimeZone();
        ev.start = QDateTime(start.date(), start.time(), zone);
        end = end.isValid() && end > start ? end : start.addSecs(3600);
        ev.end = QDateTime(end.date(), end.time(), zone);
        ev.startDay = {};
        ev.endDay = {};
        ev.exdays.clear();
    }
    const Rule::Freq freq = freqOf(f.value(QStringLiteral("repeat")).toString());
    // a rule this calendar cannot follow stays as it is until another repeat is picked
    if (freq != ev.rule.freq || (ev.rule.unsupported && !f.value(QStringLiteral("keepRule")).toBool())) {
        ev.rule = Rule{};
        ev.rule.freq = freq;
    }
    ev.hasAlarm = f.value(QStringLiteral("hasAlarm")).toBool();
    ev.alarmMinutes = f.value(QStringLiteral("alarmMinutes")).toInt();
    ev.stamp = QDateTime::currentDateTimeUtc();
    const CalFile before = m_file;
    if (i >= 0)
        m_file.events[i] = ev;
    else
        m_file.events << ev;
    // not saved: the event is not kept either, and the editor stays open
    if (!write()) {
        m_file = before;
        return QString();
    }
    return ev.uid;
}

void CalendarModel::remove(const QString &uid)
{
    const int i = indexOf(uid);
    if (i < 0)
        return;
    m_file.events.removeAt(i);
    write();
}

void CalendarModel::removeOne(const QString &uid, const QDateTime &start)
{
    const int i = indexOf(uid);
    if (i < 0)
        return;
    CalEvent &ev = m_file.events[i];
    if (ev.rule.isNull()) {
        remove(uid);
        return;
    }
    if (ev.allDay)
        ev.exdays << start.date();
    else
        ev.exdates << (ev.start.timeSpec() == Qt::TimeZone ? start.toTimeZone(ev.start.timeZone()) : start);
    write();
}

int CalendarModel::importFile(const QUrl &url)
{
    CalFile in;
    QString error;
    if (!CalStore::load(url.toLocalFile(), &in, &error)) {
        m_error = i18nc("@info", "The file could not be read: %1", error);
        Q_EMIT changed();
        return -1;
    }
    // the same event twice (same UID) is updated, not doubled
    const CalFile before = m_file;
    int leftOut = 0;
    const int added = CalStore::mergeImport(m_file, in, &leftOut);
    if (added > 0 && !write()) {
        m_file = before;
        return -1;
    }
    if (leftOut > 0) {
        m_error = i18ncp("@info", "One change to a single date of a repeating event was left out.",
                         "%1 changes to single dates of repeating events were left out.", leftOut);
        Q_EMIT changed();
    }
    return added;
}

bool CalendarModel::exportFile(const QUrl &url)
{
    QFile out(url.toLocalFile());
    if (!out.open(QIODevice::WriteOnly) || out.write(Ical::serialize(m_file)) < 0) {
        m_error = i18nc("@info", "The file could not be written: %1", out.errorString());
        Q_EMIT changed();
        return false;
    }
    return true;
}

QString CalendarModel::colorFor(int index) const
{
    return kColors.value(index);
}
