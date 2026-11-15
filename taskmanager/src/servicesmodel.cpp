// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "servicesmodel.h"
#include "autostart.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QFile>
#include <QFileInfo>
#include <QMap>

namespace Raye::TaskManager
{

namespace
{
constexpr int RefreshMs = 3000;
const QString Service = QStringLiteral("org.freedesktop.systemd1");
const QString Path = QStringLiteral("/org/freedesktop/systemd1");
const QString Manager = QStringLiteral("org.freedesktop.systemd1.Manager");

// the first process of the unit's cgroup: systemd's MainPID would cost one
// D-Bus call per unit and per refresh
int firstPid(const QString &unit)
{
    QFile procs(serviceProcsFile(unit));
    if (!procs.open(QIODevice::ReadOnly)) {
        return 0;
    }
    return procs.readLine().trimmed().toInt();
}
}

ServicesModel::ServicesModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_timer.setInterval(RefreshMs);
    connect(&m_timer, &QTimer::timeout, this, &ServicesModel::refresh);
}

int ServicesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

int ServicesModel::rowCountForQml() const
{
    return int(m_rows.size());
}

QVariant ServicesModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const Row &row = m_rows.at(index.row());
    switch (role) {
    case NameRole:
        return row.unit.chopped(8);
    case UnitRole:
        return row.unit;
    case DescriptionRole:
        return row.description;
    case StateRole:
        return row.state;
    case PidRole:
        return row.pid;
    case StartupRole:
        return row.startup;
    case BusyRole:
        return m_busy.contains(row.unit);
    }
    return {};
}

QHash<int, QByteArray> ServicesModel::roleNames() const
{
    return {{NameRole, "name"},   {UnitRole, "unit"},       {DescriptionRole, "description"}, {StateRole, "state"},
            {PidRole, "pid"},     {StartupRole, "startup"}, {BusyRole, "busy"}};
}

bool ServicesModel::active() const
{
    return m_active;
}

void ServicesModel::setActive(bool active)
{
    if (active == m_active) {
        return;
    }
    m_active = active;
    Q_EMIT activeChanged();
    // the page is the only reader: nothing is asked of systemd while it is closed
    if (active) {
        refresh();
        m_timer.start();
    } else {
        m_timer.stop();
    }
}

QString ServicesModel::filter() const
{
    return m_filter;
}

void ServicesModel::setFilter(const QString &text)
{
    if (text == m_filter) {
        return;
    }
    m_filter = text;
    Q_EMIT filterChanged();
    apply();
}

bool ServicesModel::loaded() const
{
    return m_loaded;
}

bool ServicesModel::unavailable() const
{
    return m_unavailable;
}

void ServicesModel::refresh()
{
    if (m_pending) {
        return;
    }
    m_pending = true;
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusPendingCall units = bus.asyncCall(QDBusMessage::createMethodCall(Service, Path, Manager, QStringLiteral("ListUnits")));
    const QDBusPendingCall files = bus.asyncCall(QDBusMessage::createMethodCall(Service, Path, Manager, QStringLiteral("ListUnitFiles")));
    auto *watcher = new QDBusPendingCallWatcher(files, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, units, files](QDBusPendingCallWatcher *w) mutable {
        w->deleteLater();
        units.waitForFinished();
        m_pending = false;
        QList<QStringList> unitRows;
        if (!units.isError()) {
            const QDBusArgument arg = units.reply().arguments().value(0).value<QDBusArgument>();
            arg.beginArray();
            while (!arg.atEnd()) {
                QString name, description, load, activeState, sub, following, job, jobType;
                QDBusObjectPath path, jobPath;
                uint jobId = 0;
                arg.beginStructure();
                arg >> name >> description >> load >> activeState >> sub >> following >> path >> jobId >> jobType >> jobPath;
                arg.endStructure();
                unitRows.append({name, description, load, activeState, sub});
            }
            arg.endArray();
        }
        QList<QStringList> fileRows;
        if (!files.isError()) {
            const QDBusArgument arg = files.reply().arguments().value(0).value<QDBusArgument>();
            arg.beginArray();
            while (!arg.atEnd()) {
                QString path, state;
                arg.beginStructure();
                arg >> path >> state;
                arg.endStructure();
                fileRows.append({path, state});
            }
            arg.endArray();
        }
        const bool unavailable = units.isError() && files.isError();
        if (unavailable != m_unavailable) {
            m_unavailable = unavailable;
            Q_EMIT countChanged();
        }
        m_all = mergeServices(unitRows, fileRows);
        for (Row &row : m_all) {
            if (row.state == QLatin1String("running")) {
                row.pid = firstPid(row.unit);
            }
        }
        const bool first = !m_loaded;
        m_loaded = true;
        apply();
        if (first) {
            Q_EMIT countChanged();
        }
    });
}

void ServicesModel::apply()
{
    QList<Row> rows;
    const QString text = m_filter.trimmed();
    for (const Row &row : std::as_const(m_all)) {
        if (text.isEmpty() || row.unit.contains(text, Qt::CaseInsensitive) || row.description.contains(text, Qt::CaseInsensitive)
            || (row.pid > 0 && QString::number(row.pid).contains(text))) {
            rows.append(row);
        }
    }
    bool sameShape = rows.size() == m_rows.size();
    for (int i = 0; sameShape && i < rows.size(); ++i) {
        sameShape = rows.at(i).unit == m_rows.at(i).unit;
    }
    if (sameShape) {
        m_rows = rows;
        if (!m_rows.isEmpty()) {
            Q_EMIT dataChanged(index(0), index(int(m_rows.size()) - 1));
        }
        return;
    }
    beginResetModel();
    m_rows = rows;
    endResetModel();
    Q_EMIT countChanged();
}

void ServicesModel::control(const QString &unit, const QString &verb)
{
    QString method;
    if (verb == QLatin1String("start")) {
        method = QStringLiteral("StartUnit");
    } else if (verb == QLatin1String("stop")) {
        method = QStringLiteral("StopUnit");
    } else if (verb == QLatin1String("restart")) {
        method = QStringLiteral("RestartUnit");
    } else {
        return;
    }
    QDBusMessage message = QDBusMessage::createMethodCall(Service, Path, Manager, method);
    message << unit << QStringLiteral("replace");
    message.setInteractiveAuthorizationAllowed(true);
    m_busy.insert(unit);
    apply();
    // a password prompt can stay open a while: a long wait, not the default 25 s
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(message, 5 * 60 * 1000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, unit, verb](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        m_busy.remove(unit);
        const QDBusPendingReply<QDBusObjectPath> reply = *w;
        Q_EMIT controlFinished(unit, verb, !reply.isError(), reply.isError() ? reply.error().name() : QString());
        // the job has been queued, not finished: look again shortly
        QTimer::singleShot(600, this, &ServicesModel::refresh);
        apply();
    });
}

} // namespace Raye::TaskManager
