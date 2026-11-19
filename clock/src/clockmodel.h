// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// What the window's QML reads and writes: the alarms, the timers, the
// stopwatch and the world clocks. Every change is written at once
// (ClockStore) and, for alarms and timers, raye-clock-tool is asked to arm
// the next ring. The files are watched, so what the ring tool changes
// (a one-time alarm switched off, a timer that rang) shows here too.

#pragma once

#include "countdown.h"
#include "rings.h"
#include "rowmodel.h"

#include <QFileSystemWatcher>
#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

class ClockModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(RowModel *alarms READ alarms CONSTANT)
    Q_PROPERTY(RowModel *timers READ timers CONSTANT)
    Q_PROPERTY(RowModel *cities READ cities CONSTANT)
    Q_PROPERTY(bool timersRunning READ timersRunning NOTIFY timersRunningChanged)
    Q_PROPERTY(bool stopwatchRunning READ stopwatchRunning NOTIFY stopwatchChanged)
    // the stopwatch's time = banked + (now - since) while it runs; since in ms since 1970
    Q_PROPERTY(double stopwatchBanked READ stopwatchBanked NOTIFY stopwatchChanged)
    Q_PROPERTY(double stopwatchSince READ stopwatchSince NOTIFY stopwatchChanged)
    Q_PROPERTY(QVariantList laps READ laps NOTIFY stopwatchChanged)
    Q_PROPERTY(QVariantList sounds READ sounds CONSTANT)
    Q_PROPERTY(QString timerSound READ timerSound WRITE setTimerSound NOTIFY timerSoundChanged)
    Q_PROPERTY(QString localZone READ localZone NOTIFY minuteChanged)
    // "Toronto, Canada": the place of the computer's zone
    Q_PROPERTY(QString localPlace READ localPlace NOTIFY minuteChanged)
    Q_PROPERTY(bool use12Hour READ use12Hour CONSTANT)
    // the PC can be woken from sleep for an alarm
    Q_PROPERTY(bool canWake READ canWake CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit ClockModel(QObject *parent = nullptr);

    RowModel *alarms() { return &m_alarmRows; }
    RowModel *timers() { return &m_timerRows; }
    RowModel *cities() { return &m_cityRows; }
    bool timersRunning() const;
    bool stopwatchRunning() const { return m_stopwatch.running(); }
    double stopwatchBanked() const { return double(m_stopwatch.bankedMs); }
    double stopwatchSince() const { return m_stopwatch.running() ? double(m_stopwatch.since.toMSecsSinceEpoch()) : 0; }
    QVariantList laps() const;
    QVariantList sounds() const;
    QString timerSound() const;
    void setTimerSound(const QString &id);
    QString localZone() const;
    QString localPlace() const;
    bool use12Hour() const;
    QString lastError() const { return m_error; }
    bool canWake() const;

    // alarms. fields: name, hour, minute, days, sound, snooze; an empty key makes a new one
    Q_INVOKABLE QString saveAlarm(const QString &key, const QVariantMap &fields);
    Q_INVOKABLE void removeAlarm(const QString &key);
    Q_INVOKABLE void setAlarmEnabled(const QString &key, bool on);
    Q_INVOKABLE QUrl soundUrl(const QString &id) const;

    // timers
    Q_INVOKABLE QString addTimer(const QString &name, double ms);
    Q_INVOKABLE void editTimer(const QString &key, const QString &name, double ms);
    Q_INVOKABLE void removeTimer(const QString &key);
    Q_INVOKABLE void startTimer(const QString &key);
    Q_INVOKABLE void pauseTimer(const QString &key);
    Q_INVOKABLE void resetTimer(const QString &key);

    // the stopwatch
    Q_INVOKABLE void startStopwatch();
    Q_INVOKABLE void pauseStopwatch();
    Q_INVOKABLE void lap();
    Q_INVOKABLE void resetStopwatch();

    // world clocks
    Q_INVOKABLE QVariantList searchCities(const QString &query) const;
    Q_INVOKABLE void addCity(const QString &key);
    Q_INVOKABLE void removeCity(const QString &key);

    // words
    Q_INVOKABLE QString clockText(double ms) const { return Duration::clockText(qint64(ms)); }
    Q_INVOKABLE QString preciseText(double ms) const { return Duration::preciseText(qint64(ms)); }
    Q_INVOKABLE QString ringsInText(const QDateTime &at) const;
    Q_INVOKABLE QString dayLetter(int dayOfWeek) const;
    // the time of day the way the cards write it: { clock: "3:05", meridiem: "PM" }
    Q_INVOKABLE QVariantMap timeParts(const QDateTime &t, bool seconds = false) const;
    Q_INVOKABLE QString dayName(int dayOfWeek) const;

Q_SIGNALS:
    void stopwatchChanged();
    void timersRunningChanged();
    void timerSoundChanged();
    void minuteChanged();
    void lastErrorChanged();

private:
    void load();
    void loadStopwatch();
    void refreshAlarms();
    void refreshTimers();
    void refreshCities();
    void writeAlarms();
    void writeTimers();
    void writeStopwatch();
    void writeState();
    void schedule();
    void setError(const QString &error);
    void armMinute();
    QStringList cityKeys() const;

    QList<Alarm> m_alarmList;
    QList<Countdown> m_timerList;
    RingState m_state;
    Stopwatch m_stopwatch;
    RowModel m_alarmRows;
    RowModel m_timerRows;
    RowModel m_cityRows;
    QFileSystemWatcher m_watch;
    QTimer m_reload;
    QTimer m_minute;
    QString m_error;
    bool m_timersRunning = false;
    QByteArray m_zoneId;
};
