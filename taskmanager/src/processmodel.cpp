// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "processmodel.h"
#include "efficiency.h"
#include "endtask.h"
#include "runtask.h"
#include "windowsprocess.h"

#include <KLocalizedString>

#include <QDateTime>
#include <QFileInfo>
#include <QLocale>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

#include <algorithm>
#include <csignal>
#include <fcntl.h>
#include <pwd.h>
#include <sys/resource.h>
#include <unistd.h>

namespace Raye::TaskManager
{

namespace
{
constexpr int DefaultIntervalMs = 1000;
// minimised: the figures are not looked at, the CPU time is
constexpr int IdleIntervalMs = 5000;

// Plain read(2): this runs for every process every second, and QFile's
// own bookkeeping was the larger part of the cost.
QByteArray readWhole(const QString &path)
{
    const int fd = ::open(QFile::encodeName(path).constData(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return {};
    }
    // /proc files report a size of 0: read until the end
    QByteArray out;
    char buffer[4096];
    ssize_t n = 0;
    while ((n = ::read(fd, buffer, sizeof buffer)) > 0) {
        out.append(buffer, n);
    }
    ::close(fd);
    return out;
}

// read_bytes + write_bytes of /proc/<pid>/io: what reached the disk, not
// what went through a cache. -1 when the file is refused.
qlonglong diskBytes(const QByteArray &io)
{
    if (io.isEmpty()) {
        return -1;
    }
    qlonglong total = 0;
    for (const QByteArray &line : io.split('\n')) {
        if (line.startsWith("read_bytes: ") || line.startsWith("write_bytes: ")) {
            total += line.mid(line.indexOf(' ') + 1).toLongLong();
        }
    }
    return total;
}

QString userName(uint uid, QString *fullName = nullptr)
{
    // getpwuid() reads /etc/passwd (or asks NSS) every time
    static QHash<uint, QPair<QString, QString>> known;
    const auto hit = known.constFind(uid);
    if (hit != known.constEnd()) {
        if (fullName) {
            *fullName = hit->second;
        }
        return hit->first;
    }
    QString full;
    const QString name = [&] {
        const passwd *pw = ::getpwuid(uid);
        if (!pw) {
            return QString::number(uid);
        }
        full = QString::fromLocal8Bit(pw->pw_gecos).section(QLatin1Char(','), 0, 0);
        return QString::fromLocal8Bit(pw->pw_name);
    }();
    known.insert(uid, {name, full});
    if (fullName) {
        *fullName = full;
    }
    return name;
}
}

ProcessModel::ProcessModel(QObject *parent)
    : DiffableListModel(parent)
{
    const long size = ::sysconf(_SC_PAGESIZE);
    if (size > 0) {
        m_pageSize = int(size);
    }
    m_ownUid = ::getuid();
    m_timer.setInterval(DefaultIntervalMs);
    connect(&m_timer, &QTimer::timeout, this, &ProcessModel::refresh);
    m_timer.start();
}

int ProcessModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant ProcessModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const Row &row = m_rows.at(index.row());
    switch (role) {
    case PidRole:
        return row.stat.pid;
    case NameRole:
        return row.name;
    case StateRole:
        // QLatin1Char, not QChar. `state` is a plain char (proctable.h:40,
        // one of R S D Z T t W X x K W P I), and KDECompilerSettings sets
        // QT_NO_CAST_FROM_ASCII, under which QChar(char) is `= delete`.
        // Measured 2026-11-06 on Qt 6.8 and 6.10: `use of deleted function
        // QChar::QChar(char)`. This file did not compile at all.
        return QString(QLatin1Char(row.stat.state));
    case CpuPercentRole:
        return row.cpuPercent;
    case MemoryBytesRole:
        return QVariant::fromValue(residentBytes(row.stat, m_pageSize));
    case NiceRole:
        return row.stat.nice;
    case EfficiencyRole:
        return isEfficiencyMode(row.stat);
    case StartTimeRole:
        return QVariant::fromValue(row.stat.startTime);
    case PpidRole:
        return row.stat.ppid;
    case UserRole:
        return userName(row.uid);
    case ThreadsRole:
        return row.stat.threads;
    case DiskRole:
        return row.diskRate;
    case PriorityRole:
        return int(priorityForNice(row.stat.nice));
    case AppNameRole:
        return row.appName;
    default:
        return {};
    }
}

QHash<int, QByteArray> ProcessModel::roleNames() const
{
    return {
        {PidRole, "pid"},
        {NameRole, "name"},
        {StateRole, "state"},
        {CpuPercentRole, "cpuPercent"},
        {MemoryBytesRole, "memoryBytes"},
        {NiceRole, "nice"},
        {EfficiencyRole, "efficiency"},
        {StartTimeRole, "startTime"},
        {PpidRole, "ppid"},
        {UserRole, "user"},
        {ThreadsRole, "threads"},
        {DiskRole, "disk"},
        {PriorityRole, "priority"},
        {AppNameRole, "appName"},
    };
}

bool ProcessModel::efficiencyIsReversible() const
{
    return canLeaveEfficiencyMode(rlimitNiceSoft(), hasCapSysNice());
}

int ProcessModel::intervalMs() const
{
    return m_intervalMs;
}

void ProcessModel::setIntervalMs(int ms)
{
    if (ms <= 0 || ms == m_intervalMs) {
        return;
    }
    m_intervalMs = ms;
    if (m_active) {
        m_timer.setInterval(ms);
    }
    Q_EMIT intervalMsChanged();
}

bool ProcessModel::firstSampleTaken() const
{
    return m_firstSampleTaken;
}

int ProcessModel::rowCountForQml() const
{
    return int(m_rows.size());
}

int ProcessModel::totalCount() const
{
    return int(m_all.size());
}

QString ProcessModel::filter() const
{
    return m_filter;
}

void ProcessModel::setFilter(const QString &text)
{
    if (text == m_filter) {
        return;
    }
    m_filter = text;
    Q_EMIT filterChanged();
    applyView();
}

QString ProcessModel::sortColumn() const
{
    return m_sortColumn;
}

void ProcessModel::setSortColumn(const QString &column)
{
    if (column == m_sortColumn) {
        return;
    }
    m_sortColumn = column;
    Q_EMIT sortChanged();
    applyView();
}

bool ProcessModel::sortDescending() const
{
    return m_sortDescending;
}

void ProcessModel::setSortDescending(bool descending)
{
    if (descending == m_sortDescending) {
        return;
    }
    m_sortDescending = descending;
    Q_EMIT sortChanged();
    applyView();
}

bool ProcessModel::active() const
{
    return m_active;
}

void ProcessModel::setActive(bool active)
{
    if (active == m_active) {
        return;
    }
    m_active = active;
    m_timer.setInterval(active ? m_intervalMs : IdleIntervalMs);
    Q_EMIT activeChanged();
    if (active) {
        refresh();
    }
}

bool ProcessModel::listed() const
{
    return m_listed;
}

void ProcessModel::setListed(bool listed)
{
    if (listed == m_listed) {
        return;
    }
    m_listed = listed;
    Q_EMIT listedChanged();
    applyView();
}

QVariantList ProcessModel::users() const
{
    struct Sum {
        double cpu = 0;
        qlonglong memory = 0;
        int count = 0;
    };
    QMap<uint, Sum> sums;
    for (const Row &row : m_all) {
        // the people, not the system's own accounts
        if (row.uid < 1000 || row.uid == 65534) {
            continue;
        }
        Sum &sum = sums[row.uid];
        sum.cpu += row.cpuPercent;
        sum.memory += residentBytes(row.stat, m_pageSize);
        sum.count++;
    }
    QVariantList out;
    for (auto it = sums.constBegin(); it != sums.constEnd(); ++it) {
        QString full;
        const QString name = userName(it.key(), &full);
        out.append(QVariantMap{
            {QStringLiteral("uid"), it.key()},
            {QStringLiteral("name"), name},
            {QStringLiteral("fullName"), full.isEmpty() ? name : full},
            {QStringLiteral("cpu"), it->cpu},
            {QStringLiteral("memory"), double(it->memory)},
            {QStringLiteral("count"), it->count},
            {QStringLiteral("self"), it.key() == m_ownUid},
        });
    }
    return out;
}

QList<ProcEntry> ProcessModel::entries() const
{
    QList<ProcEntry> out;
    out.reserve(m_all.size());
    for (const Row &row : m_all) {
        ProcEntry e;
        e.pid = row.stat.pid;
        e.ppid = row.stat.ppid;
        e.name = row.name;
        e.state = row.stat.state;
        e.cpu = row.cpuPercent;
        e.memory = residentBytes(row.stat, m_pageSize);
        e.disk = row.diskRate;
        e.efficiency = isEfficiencyMode(row.stat);
        e.appId = row.app.id;
        e.appName = row.appName;
        e.appIcon = row.appIcon;
        e.autostart = row.app.autostart;
        e.kernelThread = row.stat.isKernelThread();
        out.append(e);
    }
    return out;
}

ProcessModel::AppLabel ProcessModel::windowsLabel(const QString &file)
{
    const QString key = file.toLower();
    auto found = m_windowsEntries.constFind(key);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    // a program installed a moment ago gets its Start entry a little later
    if (found == m_windowsEntries.constEnd() && now - m_windowsEntriesAt > 30000) {
        m_windowsEntriesAt = now;
        m_windowsEntries.clear();
        const QString lang = QLocale().name();
        const QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
        for (auto dir = dirs.crbegin(); dir != dirs.crend(); ++dir) {
            const QStringList files = QDir(*dir).entryList({QStringLiteral("*.desktop")}, QDir::Files);
            for (const QString &f : files) {
                const DesktopEntry e = parseDesktopEntry(readWhole(*dir + QLatin1Char('/') + f), lang);
                if (e.valid && !e.hidden && !e.startupWMClass.isEmpty() && !e.name.isEmpty()) {
                    m_windowsEntries.insert(e.startupWMClass.toLower(), AppLabel{e.name, e.icon});
                }
            }
        }
        found = m_windowsEntries.constFind(key);
    }
    if (found != m_windowsEntries.constEnd()) {
        return *found;
    }
    return AppLabel{windowsProgramName(file), QStringLiteral("application-x-ms-dos-executable")};
}

ProcessModel::AppLabel ProcessModel::labelFor(const QString &id)
{
    if (id == QStringView(WindowsSupportAppId)) {
        return AppLabel{i18nc("@item what runs Windows programs, shown in place of its own processes", "Windows programs support"),
                        QStringLiteral("application-x-ms-dos-executable")};
    }
    const QString windowsFile = windowsProgramFile(id);
    if (!windowsFile.isEmpty()) {
        return windowsLabel(windowsFile);
    }
    const auto known = m_labels.constFind(id);
    if (known != m_labels.constEnd()) {
        return *known;
    }
    AppLabel label;
    const QString lang = QLocale().name();
    for (const QString &candidate : desktopIdCandidates(id)) {
        const QString path = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, candidate + QStringLiteral(".desktop"));
        const QString autostartPath = path.isEmpty()
            ? QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("autostart/") + candidate + QStringLiteral(".desktop"))
            : path;
        if (autostartPath.isEmpty()) {
            continue;
        }
        const DesktopEntry entry = parseDesktopEntry(readWhole(autostartPath), lang);
        if (entry.valid) {
            label.name = entry.name;
            label.icon = entry.icon;
            break;
        }
    }
    if (label.name.isEmpty()) {
        label.name = id.section(QLatin1Char('.'), -1);
    }
    m_labels.insert(id, label);
    return label;
}

void ProcessModel::setProcRoot(const QString &root)
{
    m_procRoot = root;
    m_previous.clear();
    m_previousTotal = 0;
    m_firstSampleTaken = false;
}

int ProcessModel::indexOfPid(int pid) const
{
    for (int i = 0; i < m_all.size(); ++i) {
        if (m_all.at(i).stat.pid == pid) {
            return i;
        }
    }
    return -1;
}

void ProcessModel::refresh()
{
    const qulonglong total = parseTotalJiffies(readWhole(m_procRoot + QStringLiteral("/stat")));
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const double seconds = m_previousAt > 0 ? double(now - m_previousAt) / 1000.0 : 0.0;

    QList<Row> rows;
    QHash<int, ProcStat> sample;
    QHash<int, Known> known;
    QHash<int, qulonglong> io;
    const QStringList entries = QDir(m_procRoot).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
    for (const QString &entry : entries) {
        bool numeric = false;
        const int pid = entry.toInt(&numeric);
        if (!numeric || pid <= 0) {
            continue;
        }
        const QString dir = m_procRoot + QLatin1Char('/') + entry;
        const ProcStat stat = parseProcStat(readWhole(dir + QStringLiteral("/stat")));
        // a process that ended between the listing and the read is not
        // an error: it simply is not in this sample
        if (!stat.valid) {
            continue;
        }
        sample.insert(pid, stat);

        Row row;
        row.stat = stat;
        const auto before = m_previous.constFind(pid);
        if (before != m_previous.constEnd() && m_previousTotal > 0) {
            row.cpuPercent = cpuPercent(*before, stat, m_previousTotal, total);
        }

        // the owner and the app do not change for the life of a process:
        // read once, kept as long as the number and start time match
        Known k;
        const auto kept = m_known.constFind(pid);
        if (kept != m_known.constEnd() && kept->startTime == stat.startTime) {
            k = *kept;
        } else {
            k.startTime = stat.startTime;
            k.uid = QFileInfo(dir).ownerId();
            k.app = appUnitFromCgroup(readWhole(dir + QStringLiteral("/cgroup")));
            const QByteArray cmdline = readWhole(dir + QStringLiteral("/cmdline"));
            k.name = stat.comm.size() < 15 ? stat.comm : displayName(stat.comm, cmdline);
            // a Windows program by its own name, and the layer's helpers as one
            // thing: never the loader's or the helpers' names
            const WindowsProcess windows = windowsProcess(QFile::symLinkTarget(dir + QStringLiteral("/exe")), cmdline);
            if (windows.role == WindowsRole::Support) {
                k.name = labelFor(QString::fromUtf16(WindowsSupportAppId)).name;
                if (!k.app.isValid() || isGenericWindowsLauncher(k.app.id)) {
                    k.app = AppUnit{QString::fromUtf16(WindowsSupportAppId), true};
                }
            } else if (windows.role == WindowsRole::Program) {
                k.name = windows.file;
                if (!k.app.isValid() || isGenericWindowsLauncher(k.app.id)) {
                    k.app = AppUnit{windowsProgramAppId(windows.file), false};
                }
            }
        }
        known.insert(pid, k);
        row.uid = k.uid;
        row.app = k.app;
        row.name = k.name;
        if (k.app.isValid()) {
            const AppLabel label = labelFor(k.app.id);
            row.appName = label.name;
            row.appIcon = label.icon;
        }

        // another user's io file is refused: the column stays empty for
        // it rather than showing a 0 that was never measured
        if (k.uid == m_ownUid) {
            const qlonglong bytes = diskBytes(readWhole(dir + QStringLiteral("/io")));
            if (bytes >= 0) {
                io.insert(pid, qulonglong(bytes));
                const auto was = m_previousIo.constFind(pid);
                row.diskRate = 0.0;
                if (was != m_previousIo.constEnd() && seconds > 0 && qulonglong(bytes) >= *was
                    && before != m_previous.constEnd() && before->startTime == stat.startTime) {
                    row.diskRate = double(qulonglong(bytes) - *was) / seconds;
                }
            }
        }
        rows.append(row);
    }

    const int was = int(m_all.size());
    m_all = rows;
    m_known = known;
    m_previous = sample;
    m_previousIo = io;
    m_previousTotal = total;
    m_previousAt = now;
    applyView();
    if (was != m_all.size()) {
        Q_EMIT countChanged();
    }
    if (!m_firstSampleTaken) {
        m_firstSampleTaken = true;
        Q_EMIT firstSampleTakenChanged();
    }
    Q_EMIT refreshed();
}

void ProcessModel::applyView()
{
    QList<Row> rows;
    const QString text = m_filter.trimmed();
    for (const Row &row : m_listed ? std::as_const(m_all) : QList<Row>()) {
        if (text.isEmpty() || row.name.contains(text, Qt::CaseInsensitive) || QString::number(row.stat.pid).contains(text)
            || row.appName.contains(text, Qt::CaseInsensitive)) {
            rows.append(row);
        }
    }
    const QString column = m_sortColumn;
    const int pageSize = m_pageSize;
    const auto key = [&](const Row &a, const Row &b) -> int {
        auto cmp = [](auto x, auto y) {
            return x < y ? -1 : (y < x ? 1 : 0);
        };
        if (column == QLatin1String("cpu")) {
            return cmp(a.cpuPercent, b.cpuPercent);
        }
        if (column == QLatin1String("memory")) {
            return cmp(residentBytes(a.stat, pageSize), residentBytes(b.stat, pageSize));
        }
        if (column == QLatin1String("pid")) {
            return cmp(a.stat.pid, b.stat.pid);
        }
        if (column == QLatin1String("threads")) {
            return cmp(a.stat.threads, b.stat.threads);
        }
        if (column == QLatin1String("priority")) {
            return cmp(-a.stat.nice, -b.stat.nice);
        }
        if (column == QLatin1String("status")) {
            return cmp(a.stat.state, b.stat.state);
        }
        if (column == QLatin1String("user")) {
            return cmp(a.uid, b.uid);
        }
        return a.name.compare(b.name, Qt::CaseInsensitive);
    };
    const bool descending = m_sortDescending;
    std::sort(rows.begin(), rows.end(), [&](const Row &a, const Row &b) {
        const int c = key(a, b);
        if (c != 0) {
            return descending ? c > 0 : c < 0;
        }
        return a.stat.pid < b.stat.pid;
    });

    const int shown = int(m_rows.size());
    replaceRows(m_rows, rows, [](const Row &r) { return qMakePair(r.stat.pid, r.stat.startTime); }, [](const Row &a, const Row &b) {
        return a.cpuPercent == b.cpuPercent && a.stat.rssPages == b.stat.rssPages && a.stat.state == b.stat.state
            && a.stat.threads == b.stat.threads && a.stat.nice == b.stat.nice && a.stat.policy == b.stat.policy;
    });
    if (shown != m_rows.size()) {
        Q_EMIT countChanged();
    }
}

const ProcessModel::Row *ProcessModel::rowOfPid(int pid) const
{
    for (const Row &row : m_all) {
        if (row.stat.pid == pid) {
            return &row;
        }
    }
    return nullptr;
}

ProcessModel::EndStep ProcessModel::endTasks(const QList<int> &pids, bool insisted)
{
    EndStep furthest = Gone;
    for (int pid : pids) {
        const EndStep step = endTask(pid, insisted);
        if (int(step) > int(furthest)) {
            furthest = step;
        }
    }
    return furthest;
}

int ProcessModel::endTree(int pid)
{
    QHash<int, int> parentOf;
    QHash<int, qulonglong> startOf;
    for (const Row &row : std::as_const(m_all)) {
        parentOf.insert(row.stat.pid, row.stat.ppid);
        startOf.insert(row.stat.pid, row.stat.startTime);
    }
    if (!startOf.contains(pid)) {
        return 0;
    }
    // the leaves first, so a parent does not restart what it lost
    QList<int> targets = descendants(pid, parentOf);
    std::reverse(targets.begin(), targets.end());
    targets.append(pid);
    int asked = 0;
    for (int target : std::as_const(targets)) {
        if (sendSignalToIdentity(ProcIdentity{target, startOf.value(target)}, SIGTERM, m_procRoot) == SignalOutcome::Sent) {
            ++asked;
        }
    }
    return asked;
}

ProcessModel::PriorityResult ProcessModel::setPriority(int pid, int priority)
{
    const Row *row = rowOfPid(pid);
    if (!row || priority < int(Priority::Low) || priority > int(Priority::High)) {
        return PriorityRefused;
    }
    const ProcIdentity identity{pid, row->stat.startTime};
    if (!readIdentity(pid, m_procRoot).sameAs(identity)) {
        return PriorityRefused;
    }
    const int nice = niceForPriority(Priority(priority));
    if (row->uid == m_ownUid && canSetNice(nice, row->stat.nice, rlimitNiceSoft(), hasCapSysNice())
        && ::setpriority(PRIO_PROCESS, id_t(pid), nice) == 0) {
        refresh();
        return PriorityApplied;
    }
    // raising a priority, or touching another user's process, needs root:
    // pkexec asks, and the process is checked again once it has
    const bool started = QProcess::startDetached(QStringLiteral("pkexec"), reniceAsRoot(identity, nice));
    return started ? PriorityAsked : PriorityRefused;
}

QString ProcessModel::runTask(const QString &command, bool asRoot)
{
    static const RunTask::Environment env = RunTask::systemEnvironment();
    return RunTask::start(RunTask::plan(command, asRoot, env), this, [this](int code, const QString &output) {
        Q_EMIT rootTaskFinished(code, output);
    });
}

QString ProcessModel::commandLine(int pid) const
{
    QByteArray raw = readWhole(m_procRoot + QStringLiteral("/%1/cmdline").arg(pid));
    if (raw.endsWith('\0')) {
        raw.chop(1);
    }
    return QString::fromLocal8Bit(raw.replace('\0', ' '));
}

ProcessModel::EndStep ProcessModel::endTask(int pid, bool insisted)
{
    const int row = indexOfPid(pid);
    if (row < 0) {
        m_ending.remove(pid);
        return Gone;
    }

    EndTaskRequest request;
    request.asked = ProcIdentity{pid, m_all.at(row).stat.startTime};
    request.current = readIdentity(pid, m_procRoot);
    request.currentState = m_all.at(row).stat.state;
    const auto ending = m_ending.constFind(pid);
    // only an attempt on THIS process counts: a number that has been
    // handed on carries no history of the one before it
    if (ending != m_ending.constEnd() && ending->startTime == request.asked.startTime) {
        request.termSent = true;
        request.msSinceTerm = QDateTime::currentMSecsSinceEpoch() - ending->termSentAt;
    }
    request.userInsisted = insisted && request.termSent;

    const EndTaskStep step = nextEndTaskStep(request);
    switch (step) {
    case EndTaskStep::Terminate:
        if (sendSignalToIdentity(request.asked, SIGTERM, m_procRoot) != SignalOutcome::Sent) {
            m_ending.remove(pid);
            return Gone;
        }
        m_ending.insert(pid, Ending{request.asked.startTime, QDateTime::currentMSecsSinceEpoch()});
        return Terminate;
    case EndTaskStep::Force:
        if (sendSignalToIdentity(request.asked, SIGKILL, m_procRoot) != SignalOutcome::Sent) {
            m_ending.remove(pid);
            return Gone;
        }
        m_ending.remove(pid);
        return Force;
    case EndTaskStep::Wait:
        return Wait;
    case EndTaskStep::OfferForce:
        return OfferForce;
    case EndTaskStep::Gone:
        m_ending.remove(pid);
        return Gone;
    }
    return Gone;
}

ProcessModel::EfficiencyResult ProcessModel::setEfficiency(int pid, bool on)
{
    const int row = indexOfPid(pid);
    if (row < 0) {
        return Refused;
    }
    switch (setEfficiencyModeChecked(m_all.at(row).stat, on, m_procRoot)) {
    case EfficiencyOutcome::Applied:
        refresh();
        return Applied;
    case EfficiencyOutcome::IoOnly:
        refresh();
        return IoOnly;
    case EfficiencyOutcome::NotReversible:
        return NotReversible;
    case EfficiencyOutcome::Refused:
        return Refused;
    }
    return Refused;
}

} // namespace Raye::TaskManager
