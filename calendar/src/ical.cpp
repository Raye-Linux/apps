// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ical.h"

#include <QRegularExpression>
#include <QTimeZone>
#include <QUuid>

namespace
{
struct Line
{
    QString name;
    QList<QPair<QString, QString>> params;
    QString value;
    QString raw;

    QString param(const QString &key) const
    {
        for (const auto &p : params) {
            if (p.first.compare(key, Qt::CaseInsensitive) == 0)
                return p.second;
        }
        return {};
    }
};

QStringList unfold(const QByteArray &data)
{
    QStringList out;
    const QString text = QString::fromUtf8(data);
    for (QString l : text.split(QLatin1Char('\n'))) {
        if (l.endsWith(QLatin1Char('\r')))
            l.chop(1);
        if ((l.startsWith(QLatin1Char(' ')) || l.startsWith(QLatin1Char('\t'))) && !out.isEmpty())
            out.last() += l.mid(1);
        else if (!l.isEmpty())
            out << l;
    }
    return out;
}

Line split(const QString &raw)
{
    Line line;
    line.raw = raw;
    bool quoted = false;
    int nameEnd = -1;
    int valueStart = -1;
    for (int i = 0; i < raw.size(); ++i) {
        const QChar c = raw[i];
        if (c == QLatin1Char('"'))
            quoted = !quoted;
        else if (!quoted && c == QLatin1Char(';') && nameEnd < 0)
            nameEnd = i;
        else if (!quoted && c == QLatin1Char(':')) {
            valueStart = i + 1;
            if (nameEnd < 0)
                nameEnd = i;
            break;
        }
    }
    if (valueStart < 0)
        return line;
    line.name = raw.left(nameEnd).toUpper();
    line.value = raw.mid(valueStart);
    const QString params = raw.mid(nameEnd, valueStart - 1 - nameEnd);
    // ;KEY=VALUE, values may be quoted
    static const QRegularExpression re(QStringLiteral(";([^=;:]+)=(\"[^\"]*\"|[^;:]*)"));
    auto it = re.globalMatch(params);
    while (it.hasNext()) {
        const auto m = it.next();
        QString v = m.captured(2);
        if (v.startsWith(QLatin1Char('"')))
            v = v.mid(1, v.size() - 2);
        line.params.append({m.captured(1).toUpper(), v});
    }
    return line;
}

QString unescape(const QString &v)
{
    QString out;
    out.reserve(v.size());
    for (int i = 0; i < v.size(); ++i) {
        if (v[i] == QLatin1Char('\\') && i + 1 < v.size()) {
            const QChar n = v[++i];
            out += (n == QLatin1Char('n') || n == QLatin1Char('N')) ? QChar(QLatin1Char('\n')) : n;
        } else {
            out += v[i];
        }
    }
    return out;
}

QString escape(const QString &v)
{
    QString out;
    for (const QChar c : v) {
        if (c == QLatin1Char('\\') || c == QLatin1Char(';') || c == QLatin1Char(','))
            out += QLatin1Char('\\') + c;
        else if (c == QLatin1Char('\n'))
            out += QStringLiteral("\\n");
        else if (c != QLatin1Char('\r'))
            out += c;
    }
    return out;
}

// lines are folded at 75 octets, never inside a UTF-8 character
void fold(QByteArray &out, const QString &line)
{
    QByteArray b = line.toUtf8();
    bool first = true;
    while (!b.isEmpty()) {
        int n = qMin<int>(first ? 75 : 74, b.size());
        while (n < b.size() && n > 0 && (uchar(b[n]) & 0xC0) == 0x80)
            --n;
        if (!first)
            out += ' ';
        out += b.left(n);
        out += "\r\n";
        b.remove(0, n);
        first = false;
    }
}

QDate parseDate(const QString &v)
{
    return QDate::fromString(v.left(8), QStringLiteral("yyyyMMdd"));
}

QString dateText(const QDate &d)
{
    return d.toString(QStringLiteral("yyyyMMdd"));
}

QString dateTimeText(const QDateTime &dt)
{
    if (dt.timeSpec() == Qt::UTC || (dt.timeSpec() == Qt::OffsetFromUTC && dt.offsetFromUtc() == 0))
        return dt.toUTC().toString(QStringLiteral("yyyyMMdd'T'HHmmss'Z'"));
    return dt.toString(QStringLiteral("yyyyMMdd'T'HHmmss"));
}

// ";TZID=Europe/Paris:20261111T143000", ":20261111T143000Z" or ":..." floating
QString dateTimeProperty(const QString &name, const QDateTime &dt)
{
    if (dt.timeSpec() == Qt::TimeZone)
        return name + QStringLiteral(";TZID=") + QString::fromUtf8(dt.timeZone().id()) + QLatin1Char(':') + dateTimeText(dt);
    return name + QLatin1Char(':') + dateTimeText(dt);
}

// [+-]P[nW][nD][T[nH][nM][nS]], in seconds
bool parseDuration(const QString &v, qint64 *secs)
{
    static const QRegularExpression re(QStringLiteral(
        "^([+-])?P(?:(\\d+)W)?(?:(\\d+)D)?(?:T(?:(\\d+)H)?(?:(\\d+)M)?(?:(\\d+)S)?)?$"));
    const auto m = re.match(v.trimmed().toUpper());
    if (!m.hasMatch())
        return false;
    qint64 s = m.captured(2).toLongLong() * 7 * 86400 + m.captured(3).toLongLong() * 86400
        + m.captured(4).toLongLong() * 3600 + m.captured(5).toLongLong() * 60 + m.captured(6).toLongLong();
    *secs = m.captured(1) == QLatin1String("-") ? -s : s;
    return true;
}

QString durationText(qint64 secs)
{
    QString sign;
    if (secs < 0) {
        sign = QStringLiteral("-");
        secs = -secs;
    }
    if (secs == 0)
        return QStringLiteral("PT0S");
    QString t = sign + QLatin1Char('P');
    if (secs % 86400 == 0)
        return t + QString::number(secs / 86400) + QLatin1Char('D');
    if (secs >= 86400) {
        t += QString::number(secs / 86400) + QLatin1Char('D');
        secs %= 86400;
    }
    t += QLatin1Char('T');
    if (secs >= 3600) {
        t += QString::number(secs / 3600) + QLatin1Char('H');
        secs %= 3600;
    }
    if (secs >= 60) {
        t += QString::number(secs / 60) + QLatin1Char('M');
        secs %= 60;
    }
    if (secs)
        t += QString::number(secs) + QLatin1Char('S');
    return t;
}

const char *const kDays[] = {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};

int dayNumber(const QString &d)
{
    for (int i = 0; i < 7; ++i) {
        if (d == QLatin1String(kDays[i]))
            return i + 1;
    }
    return 0;
}

void readAlarm(CalEvent &ev, const QList<Line> &lines)
{
    for (const Line &l : lines) {
        if (l.name != QLatin1String("TRIGGER"))
            continue;
        if (l.param(QStringLiteral("VALUE")).compare(QLatin1String("DATE-TIME"), Qt::CaseInsensitive) == 0) {
            const QDateTime at = Ical::parseDateTime(l.value, {});
            const QDateTime start = ev.allDay ? QDateTime(ev.startDay, QTime(0, 0)) : ev.start;
            if (at.isValid() && start.isValid()) {
                ev.hasAlarm = true;
                ev.alarmMinutes = int(at.secsTo(start) / 60);
            }
            return;
        }
        qint64 secs = 0;
        if (!parseDuration(l.value, &secs))
            return;
        if (l.param(QStringLiteral("RELATED")).compare(QLatin1String("END"), Qt::CaseInsensitive) == 0)
            secs += ev.lengthSecs();
        ev.hasAlarm = true;
        ev.alarmMinutes = int(-secs / 60);
        return;
    }
}

CalEvent readEvent(const QStringList &body)
{
    CalEvent ev;
    QList<Line> alarm;
    bool inAlarm = false;
    bool alarmTaken = false;
    int depth = 0; // other nested components, kept raw
    QString dtendValue, dtendTz, durationValue;
    bool endIsDate = false;
    QStringList pendingAlarm;

    for (const QString &raw : body) {
        const Line l = split(raw);
        if (depth > 0) {
            ev.extra << raw;
            if (l.name == QLatin1String("BEGIN"))
                ++depth;
            else if (l.name == QLatin1String("END"))
                --depth;
            continue;
        }
        if (inAlarm) {
            pendingAlarm << raw;
            if (l.name == QLatin1String("END") && l.value.compare(QLatin1String("VALARM"), Qt::CaseInsensitive) == 0) {
                inAlarm = false;
                // only the first alarm is read; the others travel as they are
                if (alarmTaken)
                    ev.extra << pendingAlarm;
                alarmTaken = true;
                pendingAlarm.clear();
            } else if (!alarmTaken) {
                alarm << l;
            }
            continue;
        }
        if (l.name == QLatin1String("BEGIN")) {
            if (l.value.compare(QLatin1String("VALARM"), Qt::CaseInsensitive) == 0) {
                inAlarm = true;
                pendingAlarm = {raw};
            } else {
                ev.extra << raw;
                depth = 1;
            }
            continue;
        }
        const QString tz = l.param(QStringLiteral("TZID"));
        const bool isDate = l.param(QStringLiteral("VALUE")).compare(QLatin1String("DATE"), Qt::CaseInsensitive) == 0
            || (l.value.size() == 8 && !l.value.contains(QLatin1Char('T')));
        if (l.name == QLatin1String("UID")) {
            ev.uid = l.value;
        } else if (l.name == QLatin1String("SUMMARY")) {
            ev.summary = unescape(l.value);
        } else if (l.name == QLatin1String("LOCATION")) {
            ev.location = unescape(l.value);
        } else if (l.name == QLatin1String("DESCRIPTION")) {
            ev.description = unescape(l.value);
        } else if (l.name == QLatin1String("X-RAYE-COLOR")) {
            ev.color = l.value;
        } else if (l.name == QLatin1String("DTSTAMP")) {
            ev.stamp = Ical::parseDateTime(l.value, {});
        } else if (l.name == QLatin1String("DTSTART")) {
            if (isDate) {
                ev.allDay = true;
                ev.startDay = parseDate(l.value);
            } else {
                ev.start = Ical::parseDateTime(l.value, tz);
            }
        } else if (l.name == QLatin1String("DTEND")) {
            dtendValue = l.value;
            dtendTz = tz;
            endIsDate = isDate;
        } else if (l.name == QLatin1String("DURATION")) {
            durationValue = l.value;
        } else if (l.name == QLatin1String("RRULE")) {
            ev.rule = Ical::parseRule(l.value);
        } else if (l.name == QLatin1String("EXDATE")) {
            for (const QString &v : l.value.split(QLatin1Char(','))) {
                if (isDate)
                    ev.exdays << parseDate(v);
                else
                    ev.exdates << Ical::parseDateTime(v, tz);
            }
        } else if (l.name != QLatin1String("END")) {
            ev.extra << raw;
        }
    }

    qint64 dur = 0;
    const bool hasDuration = !durationValue.isEmpty() && parseDuration(durationValue, &dur);
    if (ev.allDay) {
        if (!dtendValue.isEmpty() && endIsDate)
            ev.endDay = parseDate(dtendValue);
        else if (hasDuration)
            ev.endDay = ev.startDay.addDays(qMax<qint64>(1, dur / 86400));
        if (!ev.endDay.isValid() || ev.endDay <= ev.startDay)
            ev.endDay = ev.startDay.addDays(1);
        // all-day exceptions may come as date-times
        for (const QDateTime &d : std::as_const(ev.exdates))
            ev.exdays << d.date();
        ev.exdates.clear();
    } else {
        if (!dtendValue.isEmpty())
            ev.end = endIsDate ? QDateTime(parseDate(dtendValue), QTime(0, 0), ev.start.timeZone())
                               : Ical::parseDateTime(dtendValue, dtendTz);
        else if (hasDuration)
            ev.end = ev.start.addSecs(dur);
        if (!ev.end.isValid() || ev.end < ev.start)
            ev.end = ev.start;
    }
    readAlarm(ev, alarm);
    return ev;
}

void writeEvent(QByteArray &out, const CalEvent &ev)
{
    fold(out, QStringLiteral("BEGIN:VEVENT"));
    fold(out, QStringLiteral("UID:") + ev.uid);
    const QDateTime stamp = ev.stamp.isValid() ? ev.stamp : QDateTime::currentDateTimeUtc();
    fold(out, QStringLiteral("DTSTAMP:") + dateTimeText(stamp.toUTC()));
    if (ev.allDay) {
        fold(out, QStringLiteral("DTSTART;VALUE=DATE:") + dateText(ev.startDay));
        fold(out, QStringLiteral("DTEND;VALUE=DATE:") + dateText(ev.endDay));
    } else {
        fold(out, dateTimeProperty(QStringLiteral("DTSTART"), ev.start));
        fold(out, dateTimeProperty(QStringLiteral("DTEND"), ev.end));
    }
    fold(out, QStringLiteral("SUMMARY:") + escape(ev.summary));
    if (!ev.location.isEmpty())
        fold(out, QStringLiteral("LOCATION:") + escape(ev.location));
    if (!ev.description.isEmpty())
        fold(out, QStringLiteral("DESCRIPTION:") + escape(ev.description));
    if (!ev.color.isEmpty())
        fold(out, QStringLiteral("X-RAYE-COLOR:") + ev.color);
    if (!ev.rule.isNull())
        fold(out, QStringLiteral("RRULE:") + Ical::ruleText(ev.rule));
    for (const QDate &d : ev.exdays)
        fold(out, QStringLiteral("EXDATE;VALUE=DATE:") + dateText(d));
    for (const QDateTime &d : ev.exdates)
        fold(out, dateTimeProperty(QStringLiteral("EXDATE"), d));
    for (const QString &l : ev.extra)
        fold(out, l);
    if (ev.hasAlarm) {
        fold(out, QStringLiteral("BEGIN:VALARM"));
        fold(out, QStringLiteral("ACTION:DISPLAY"));
        fold(out, QStringLiteral("DESCRIPTION:") + escape(ev.summary.isEmpty() ? QStringLiteral("Reminder") : ev.summary));
        fold(out, QStringLiteral("TRIGGER:") + durationText(-qint64(ev.alarmMinutes) * 60));
        fold(out, QStringLiteral("END:VALARM"));
    }
    fold(out, QStringLiteral("END:VEVENT"));
}
} // namespace

qint64 CalEvent::lengthSecs() const
{
    if (allDay)
        return qint64(startDay.daysTo(endDay)) * 86400;
    return start.secsTo(end);
}

QDateTime Ical::parseDateTime(const QString &value, const QString &tzid)
{
    const QString v = value.trimmed();
    if (v.size() < 15)
        return {};
    const QDate d = parseDate(v);
    const QTime t = QTime::fromString(v.mid(9, 6), QStringLiteral("HHmmss"));
    if (!d.isValid() || !t.isValid())
        return {};
    if (v.endsWith(QLatin1Char('Z')))
        return QDateTime(d, t, QTimeZone::UTC);
    if (!tzid.isEmpty()) {
        QTimeZone zone(tzid.toUtf8());
        // calendars made on Windows name zones the Windows way
        if (!zone.isValid())
            zone = QTimeZone(QTimeZone::windowsIdToDefaultIanaId(tzid.toUtf8()));
        if (zone.isValid())
            return QDateTime(d, t, zone);
    }
    return QDateTime(d, t);
}

Rule Ical::parseRule(const QString &text)
{
    Rule r;
    r.raw = text;
    for (const QString &part : text.split(QLatin1Char(';'), Qt::SkipEmptyParts)) {
        const QString key = part.section(QLatin1Char('='), 0, 0).toUpper();
        const QString val = part.section(QLatin1Char('='), 1).toUpper();
        if (key == QLatin1String("FREQ")) {
            r.freq = val == QLatin1String("DAILY")   ? Rule::Daily
                : val == QLatin1String("WEEKLY")     ? Rule::Weekly
                : val == QLatin1String("MONTHLY")    ? Rule::Monthly
                : val == QLatin1String("YEARLY")     ? Rule::Yearly
                                                     : Rule::None;
        } else if (key == QLatin1String("INTERVAL")) {
            r.interval = qMax(1, val.toInt());
        } else if (key == QLatin1String("COUNT")) {
            r.count = qMax(0, val.toInt());
        } else if (key == QLatin1String("UNTIL")) {
            if (val.size() == 8)
                r.until = QDateTime(parseDate(val), QTime(23, 59, 59));
            else
                r.until = parseDateTime(val, {});
        } else if (key == QLatin1String("BYDAY")) {
            static const QRegularExpression re(QStringLiteral("^([+-]?\\d+)?([A-Z]{2})$"));
            for (const QString &d : val.split(QLatin1Char(','))) {
                const auto m = re.match(d);
                if (!m.hasMatch()) {
                    r.unsupported = true;
                    continue;
                }
                if (const int n = dayNumber(m.captured(2))) {
                    r.byDay << n;
                    if (!m.captured(1).isEmpty())
                        r.byDayNth = m.captured(1).toInt();
                }
            }
        } else if (key == QLatin1String("BYMONTHDAY")) {
            r.byMonthDay = val.section(QLatin1Char(','), 0, 0).toInt();
            r.unsupported = r.unsupported || val.contains(QLatin1Char(','));
        } else if (key != QLatin1String("WKST")) {
            r.unsupported = true;
        }
    }
    // a day of the week only goes with a week, or with one "nth" in a month
    if (!r.byDay.isEmpty() && r.freq != Rule::Weekly
        && !(r.freq == Rule::Monthly && r.byDay.size() == 1 && r.byDayNth != 0))
        r.unsupported = true;
    if (r.byDayNth != 0 && r.freq != Rule::Monthly)
        r.unsupported = true;
    if (r.byMonthDay != 0 && r.freq != Rule::Monthly)
        r.unsupported = true;
    return r;
}

QString Ical::ruleText(const Rule &rule)
{
    // a rule read from a file and not changed goes back as it came
    if (!rule.raw.isEmpty() && parseRule(rule.raw) == rule)
        return rule.raw;
    static const char *const freqs[] = {"", "DAILY", "WEEKLY", "MONTHLY", "YEARLY"};
    QString t = QStringLiteral("FREQ=") + QLatin1String(freqs[rule.freq]);
    if (rule.interval > 1)
        t += QStringLiteral(";INTERVAL=") + QString::number(rule.interval);
    if (rule.count > 0)
        t += QStringLiteral(";COUNT=") + QString::number(rule.count);
    else if (rule.until.isValid())
        t += QStringLiteral(";UNTIL=") + dateTimeText(rule.until.toUTC());
    if (!rule.byDay.isEmpty()) {
        QStringList days;
        for (int d : rule.byDay)
            days << (rule.byDayNth ? QString::number(rule.byDayNth) : QString()) + QLatin1String(kDays[(d - 1) % 7]);
        t += QStringLiteral(";BYDAY=") + days.join(QLatin1Char(','));
    }
    if (rule.byMonthDay)
        t += QStringLiteral(";BYMONTHDAY=") + QString::number(rule.byMonthDay);
    return t;
}

QString Ical::newUid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral("@raye-calendar");
}

CalFile Ical::parse(const QByteArray &data)
{
    CalFile file;
    const QStringList lines = unfold(data);
    QStringList body;
    QStringList other;
    int otherDepth = 0;
    bool inEvent = false;
    for (const QString &raw : lines) {
        const Line l = split(raw);
        const bool begin = l.name == QLatin1String("BEGIN");
        const bool end = l.name == QLatin1String("END");
        if (inEvent) {
            if (end && l.value.compare(QLatin1String("VEVENT"), Qt::CaseInsensitive) == 0) {
                inEvent = false;
                CalEvent ev = readEvent(body);
                if (!ev.allDay && !ev.start.isValid())
                    continue;
                if (ev.allDay && !ev.startDay.isValid())
                    continue;
                if (ev.uid.isEmpty())
                    ev.uid = newUid();
                file.events << ev;
            } else {
                body << raw;
            }
            continue;
        }
        if (otherDepth > 0) {
            other << raw;
            if (begin)
                ++otherDepth;
            else if (end)
                --otherDepth;
            continue;
        }
        if (begin && l.value.compare(QLatin1String("VEVENT"), Qt::CaseInsensitive) == 0) {
            inEvent = true;
            body.clear();
        } else if (begin && l.value.compare(QLatin1String("VCALENDAR"), Qt::CaseInsensitive) != 0) {
            other << raw;
            otherDepth = 1;
        }
    }
    file.other = other;
    return file;
}

QByteArray Ical::serialize(const CalFile &file)
{
    QByteArray out;
    fold(out, QStringLiteral("BEGIN:VCALENDAR"));
    fold(out, QStringLiteral("VERSION:2.0"));
    fold(out, QStringLiteral("PRODID:-//Raye Linux//Raye Calendar//EN"));
    fold(out, QStringLiteral("CALSCALE:GREGORIAN"));
    for (const QString &l : file.other)
        fold(out, l);
    for (const CalEvent &ev : file.events)
        writeEvent(out, ev);
    fold(out, QStringLiteral("END:VCALENDAR"));
    return out;
}
