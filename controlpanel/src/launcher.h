// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QObject>
#include <QQmlEngine>
#include <QSet>
#include <QStringList>
#include <QTimer>

// Opens what an item of the Control Panel points at: a page of Raye
// Settings (by its id in Settings' layout.json), or a program.
class Launcher : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    // something was just started and has no window yet: the pointer is the
    // busy one, as when Windows starts a program
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    // how long busy lasts at most when the program never takes the focus
    Q_PROPERTY(int busyTimeout READ busyTimeout WRITE setBusyTimeout NOTIFY busyChanged)

public:
    explicit Launcher(QObject *parent = nullptr);

    // the page ids Settings has, read from its layout.json; with no
    // layout at all every page counts as there, Settings will say if not
    static QSet<QString> pagesOf(const QByteArray &layoutJson);
    // page id -> what it needs ("wifi", "bluetooth|wifi"...), as Settings
    // hides those pages on a PC without it
    static QHash<QString, QString> requirementsOf(const QByteArray &layoutJson);
    static QStringList settingsArguments(const QString &page);
    // Settings' own checks, read from the files under root
    static bool probe(const QString &root, const QString &requirement);

    Q_INVOKABLE bool hasPage(const QString &page) const;
    Q_INVOKABLE bool has(const QString &requirement) const;
    Q_INVOKABLE void openPage(const QString &page);
    Q_INVOKABLE bool hasProgram(const QString &program) const;
    Q_INVOKABLE QString firstProgram(const QStringList &programs) const;
    Q_INVOKABLE void run(const QString &program, const QStringList &arguments = {});

    // the desktop file of a program, for the launch feedback ("" if none)
    static QString desktopNameFor(const QString &program);

    bool busy() const { return m_busy; }
    int busyTimeout() const { return m_busyTimer.interval(); }
    void setBusyTimeout(int ms);

Q_SIGNALS:
    void busyChanged();

private:
    void start(const QString &program, const QStringList &arguments, const QString &desktopName);
    void setBusy(bool busy);

    bool m_busy = false;
    QTimer m_busyTimer;
    // the answers do not change while the window is open, and the views
    // ask for them again and again
    mutable QHash<QString, bool> m_has;
    mutable QHash<QString, bool> m_programs;
    QSet<QString> m_pages;
    QHash<QString, QString> m_needs;
    QString m_root;
    bool m_known = false;
};
