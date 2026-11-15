// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The Services page: the system's services, from systemd over D-Bus.
//
// Starting, stopping and restarting call the manager directly, never
// systemctl, and allow interactive authorisation: polkit then asks for an
// administrator's password (manage-units is auth_admin_keep for an
// active session) and a refusal comes back as an error the page shows,
// rather than as nothing happening.

#pragma once

#include "services.h"

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QSet>
#include <QTimer>

namespace Raye::TaskManager
{

class ServicesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int count READ rowCountForQml NOTIFY countChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY countChanged)
    // systemd did not answer: the page says so instead of an empty list
    Q_PROPERTY(bool unavailable READ unavailable NOTIFY countChanged)

public:
    enum Role { NameRole = Qt::UserRole + 1, UnitRole, DescriptionRole, StateRole, PidRole, StartupRole, BusyRole };

    explicit ServicesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCountForQml() const;

    bool active() const;
    void setActive(bool active);
    QString filter() const;
    void setFilter(const QString &text);
    bool loaded() const;
    bool unavailable() const;

    Q_INVOKABLE void refresh();
    // verb is "start", "stop" or "restart"
    Q_INVOKABLE void control(const QString &unit, const QString &verb);

Q_SIGNALS:
    void activeChanged();
    void filterChanged();
    void countChanged();
    // ok false: message is what systemd or polkit said
    void controlFinished(const QString &unit, const QString &verb, bool ok, const QString &message);

private:
    using Row = ServiceRow;

    void apply();

    QList<Row> m_all;
    QList<Row> m_rows;
    QSet<QString> m_busy;
    QString m_filter;
    bool m_active = false;
    bool m_loaded = false;
    bool m_unavailable = false;
    bool m_pending = false;
    QTimer m_timer;
};

} // namespace Raye::TaskManager
