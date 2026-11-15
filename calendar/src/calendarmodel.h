// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// What the window's QML reads and writes: the occurrences of a range of
// days, one event to edit, and saving, deleting, import and export.
// Every save rewrites the whole file (CalStore) and asks
// raye-calendar-tool to arm the timer of the next reminder.

#pragma once

#include "ical.h"

#include <QObject>
#include <QQmlEngine>
#include <QUrl>
#include <QVariantList>

class CalendarModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    // bumps on every change, so bindings that read occurrences() redo it
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QString holidayRegion READ holidayRegion WRITE setHolidayRegion NOTIFY changed)
    Q_PROPERTY(QVariantList holidayRegions READ holidayRegions CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY changed)

public:
    explicit CalendarModel(QObject *parent = nullptr);

    int revision() const { return m_revision; }
    QString holidayRegion() const { return m_region; }
    void setHolidayRegion(const QString &region);
    QVariantList holidayRegions() const;
    QString lastError() const { return m_error; }

    // [from, to): { uid, title, start, end, allDay, color, location, holiday, repeats, alarm }
    Q_INVOKABLE QVariantList occurrences(const QDateTime &from, const QDateTime &to) const;
    Q_INVOKABLE QVariantMap event(const QString &uid) const;
    // fields as event() gives them; an empty uid makes a new event. Returns its uid.
    Q_INVOKABLE QString save(const QVariantMap &fields);
    Q_INVOKABLE void remove(const QString &uid);
    // one occurrence of a repeating event
    Q_INVOKABLE void removeOne(const QString &uid, const QDateTime &start);
    Q_INVOKABLE int importFile(const QUrl &url);
    Q_INVOKABLE bool exportFile(const QUrl &url);
    Q_INVOKABLE QString colorFor(int index) const;

Q_SIGNALS:
    void changed();

private:
    void reload();
    bool write();
    int indexOf(const QString &uid) const;

    CalFile m_file;
    QString m_region;
    QString m_error;
    int m_revision = 0;
};
