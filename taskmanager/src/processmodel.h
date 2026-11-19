// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The rows of the Processes tab, and the two actions the window offers
// on a row. The reading and the arithmetic are in proctable.h,
// endtask.h and efficiency.h, which the tests link on their own; this
// class is the part that needs Qt's object system, and it holds the one
// thing a pure function cannot: the previous sample.
//
// The processor column needs two readings, so the first refresh after
// the window opens shows 0 % everywhere. That is right, and saying "the
// first figure arrives in a second" is better than inventing one.

#pragma once

#include "appgroups.h"
#include "proctable.h"

#include "rowdiff.h"

#include <QAbstractListModel>
#include <QHash>
#include <QQmlEngine>
#include <QTimer>
#include <QVariantList>

namespace Raye::TaskManager
{

class ProcessModel : public DiffableListModel
{
    Q_OBJECT
    QML_ELEMENT

    // false when the kernel will not let this user take a process back
    // out of the efficiency mode; the window then asks before putting
    // one in, and draws no switch. See efficiency.h.
    Q_PROPERTY(bool efficiencyIsReversible READ efficiencyIsReversible CONSTANT)
    Q_PROPERTY(int intervalMs READ intervalMs WRITE setIntervalMs NOTIFY intervalMsChanged)
    Q_PROPERTY(bool firstSampleTaken READ firstSampleTaken NOTIFY firstSampleTakenChanged)
    // QAbstractListModel has no count a QML binding can read
    Q_PROPERTY(int count READ rowCountForQml NOTIFY countChanged)
    // every process, whatever the filter: the Processes page counts these
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countChanged)
    // the rows of this model only; the grouped view has its own filter
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    // "name", "pid", "status", "user", "cpu", "memory", "threads", "priority"
    Q_PROPERTY(QString sortColumn READ sortColumn WRITE setSortColumn NOTIFY sortChanged)
    Q_PROPERTY(bool sortDescending READ sortDescending WRITE setSortDescending NOTIFY sortChanged)
    // one entry per person with processes: uid, name, fullName, cpu, memory, count
    Q_PROPERTY(QVariantList users READ users NOTIFY refreshed)
    // false while the Details page is not shown: the flat rows are then
    // neither filtered nor sorted
    Q_PROPERTY(bool listed READ listed WRITE setListed NOTIFY listedChanged)
    // false while the window is minimised: one reading every 5 seconds
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)

public:
    enum Role {
        PidRole = Qt::UserRole + 1,
        NameRole,
        StateRole, // one letter, as the kernel gives it
        CpuPercentRole,
        MemoryBytesRole,
        NiceRole,
        EfficiencyRole,
        StartTimeRole,
        PpidRole,
        UserRole,
        ThreadsRole,
        DiskRole, // bytes per second, -1 for another user's process
        PriorityRole, // a Priority, as an int
        AppNameRole,
    };

    // mirrors of the enums in endtask.h and efficiency.h, for QML
    enum EndStep { Gone, Terminate, Wait, OfferForce, Force };
    Q_ENUM(EndStep)
    enum EfficiencyResult { Applied, IoOnly, Refused, NotReversible };
    Q_ENUM(EfficiencyResult)
    enum PriorityResult { PriorityApplied, PriorityAsked, PriorityRefused };
    Q_ENUM(PriorityResult)

    explicit ProcessModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool efficiencyIsReversible() const;
    int intervalMs() const;
    void setIntervalMs(int ms);
    bool firstSampleTaken() const;
    int rowCountForQml() const;
    int totalCount() const;
    QString filter() const;
    void setFilter(const QString &text);
    QString sortColumn() const;
    void setSortColumn(const QString &column);
    bool sortDescending() const;
    void setSortDescending(bool descending);
    QVariantList users() const;
    bool active() const;
    void setActive(bool active);
    bool listed() const;
    void setListed(bool listed);

    // what the grouped view is built from: every process, unfiltered
    QList<ProcEntry> entries() const;

    // the /proc tree to read; "/proc" unless a test says otherwise
    void setProcRoot(const QString &root);

    Q_INVOKABLE void refresh();

    // Asks for the next step of ending pid and takes it. insisted is
    // true only when the user has pressed the force button of the
    // sheet. Returns the step that was taken, for the window to show.
    Q_INVOKABLE EndStep endTask(int pid, bool insisted);

    Q_INVOKABLE EfficiencyResult setEfficiency(int pid, bool on);

    // The same request for several processes at once, an app's group.
    // Returns the furthest step any of them reached.
    Q_INVOKABLE EndStep endTasks(const QList<int> &pids, bool insisted);

    // Asks pid and everything it started to close. Returns how many
    // processes were asked.
    Q_INVOKABLE int endTree(int pid);

    // priority is a Priority. Lowering is done here; raising needs an
    // administrator and goes through pkexec, which shows its own prompt.
    Q_INVOKABLE PriorityResult setPriority(int pid, int priority);

    Q_INVOKABLE QString commandLine(int pid) const;

    // Starts command, split like a shell would (no shell is involved).
    // "started", "notfound", "window" (as root: it would have no display),
    // or "waiting" for the root password: rootTaskFinished() says the rest
    Q_INVOKABLE QString runTask(const QString &command, bool asRoot);

Q_SIGNALS:
    // code: the program's, or pkexec's 126 (no password) and 127 (not allowed)
    void rootTaskFinished(int code, const QString &output);
    void intervalMsChanged();
    void firstSampleTakenChanged();
    void countChanged();
    void filterChanged();
    void sortChanged();
    void activeChanged();
    void listedChanged();
    void refreshed();

private:
    struct Row {
        ProcStat stat;
        double cpuPercent = 0.0;
        uint uid = 0;
        double diskRate = -1.0;
        AppUnit app;
        QString name;
        QString appName;
        QString appIcon;
    };
    // what does not change for the life of a process
    struct Known {
        qulonglong startTime = 0;
        uint uid = 0;
        AppUnit app;
        QString name;
    };
    struct AppLabel {
        QString name;
        QString icon;
    };
    // what was asked of a process, and when: the grace of endtask.h is
    // counted from here
    struct Ending {
        qulonglong startTime = 0;
        qint64 termSentAt = 0;
    };

    QList<Row> m_all;
    QList<Row> m_rows;
    QHash<int, Known> m_known;
    QHash<int, qulonglong> m_previousIo;
    QHash<QString, AppLabel> m_labels;
    // Start entries of Windows programs, by StartupWMClass (the .exe name)
    QHash<QString, AppLabel> m_windowsEntries;
    qint64 m_windowsEntriesAt = 0;
    qint64 m_previousAt = 0;
    QString m_filter;
    QString m_sortColumn = QStringLiteral("name");
    bool m_sortDescending = false;
    bool m_active = true;
    bool m_listed = true;
    int m_intervalMs = 1000;
    uint m_ownUid = 0;
    QHash<int, ProcStat> m_previous;
    QHash<int, Ending> m_ending;
    qulonglong m_previousTotal = 0;
    bool m_firstSampleTaken = false;
    int m_pageSize = 4096;
    QString m_procRoot = QStringLiteral("/proc");
    QTimer m_timer;

    int indexOfPid(int pid) const;
    const Row *rowOfPid(int pid) const;
    void applyView();
    AppLabel labelFor(const QString &id);
    AppLabel windowsLabel(const QString &file);
};

} // namespace Raye::TaskManager
