// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "launcher.h"

#include <KApplicationTrader>
#include <KIO/CommandLauncherJob>
#include <KService>

#include <QCursor>
#include <QDir>
#include <QGuiApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {
QJsonArray categoriesOf(const QByteArray &layoutJson)
{
    return QJsonDocument::fromJson(layoutJson).object().value(QStringLiteral("categories")).toArray();
}

QStringList subfolders(const QString &path, const QStringList &filter = {})
{
    return QDir(path).entryList(filter, QDir::Dirs | QDir::NoDotAndDotDot);
}

// whole words, so "pen" is in "Wacom Pen" and not in "OpenRGB"
bool anInputIsNamed(const QString &sys, const QString &words)
{
    const QRegularExpression word(QStringLiteral("\\b(%1)\\b").arg(words));
    const QString input = sys + QStringLiteral("/class/input/");
    const QStringList entries = subfolders(input, {QStringLiteral("input*")});
    for (const QString &entry : entries) {
        QFile f(input + entry + QStringLiteral("/name"));
        if (f.open(QIODevice::ReadOnly) && word.match(QString::fromUtf8(f.readLine()).trimmed().toLower()).hasMatch())
            return true;
    }
    return false;
}
} // namespace

Launcher::Launcher(QObject *parent)
    : QObject(parent)
    , m_root(qEnvironmentVariable("RAYE_SYSTEM_ROOT"))
{
    m_busyTimer.setSingleShot(true);
    m_busyTimer.setInterval(5000);
    connect(&m_busyTimer, &QTimer::timeout, this, [this] { setBusy(false); });
    // the program's window took the focus: it is there
    if (qGuiApp) {
        connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
            if (state != Qt::ApplicationActive)
                setBusy(false);
        });
    }
    QString path = qEnvironmentVariable("RAYE_SETTINGS_LAYOUT");
    if (path.isEmpty())
        path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("raye-settings/layout.json"));
    QFile f(path);
    if (!path.isEmpty() && f.open(QIODevice::ReadOnly)) {
        const QByteArray layout = f.readAll();
        m_pages = pagesOf(layout);
        m_needs = requirementsOf(layout);
        m_known = !m_pages.isEmpty();
    }
}

QSet<QString> Launcher::pagesOf(const QByteArray &layoutJson)
{
    QSet<QString> pages;
    const QJsonArray categories = categoriesOf(layoutJson);
    for (const QJsonValue &c : categories) {
        const QJsonObject category = c.toObject();
        pages.insert(category.value(QStringLiteral("id")).toString());
        const QJsonArray list = category.value(QStringLiteral("pages")).toArray();
        for (const QJsonValue &p : list)
            pages.insert(p.toObject().value(QStringLiteral("id")).toString());
    }
    pages.remove(QString());
    return pages;
}

QHash<QString, QString> Launcher::requirementsOf(const QByteArray &layoutJson)
{
    QHash<QString, QString> needs;
    const QJsonArray categories = categoriesOf(layoutJson);
    for (const QJsonValue &c : categories) {
        const QJsonArray list = c.toObject().value(QStringLiteral("pages")).toArray();
        for (const QJsonValue &p : list) {
            const QJsonObject page = p.toObject();
            const QString requirement = page.value(QStringLiteral("requires")).toString();
            if (!requirement.isEmpty())
                needs.insert(page.value(QStringLiteral("id")).toString(), requirement);
        }
    }
    return needs;
}

QStringList Launcher::settingsArguments(const QString &page)
{
    return {QStringLiteral("--page"), page};
}

bool Launcher::probe(const QString &root, const QString &requirement)
{
    if (requirement.contains(QLatin1Char('|'))) {
        const QStringList either = requirement.split(QLatin1Char('|'), Qt::SkipEmptyParts);
        return std::any_of(either.cbegin(), either.cend(), [&root](const QString &one) { return probe(root, one); });
    }
    const QString sys = root + QStringLiteral("/sys");
    if (requirement == QLatin1String("wifi")) {
        const QString net = sys + QStringLiteral("/class/net/");
        const QStringList entries = subfolders(net);
        return std::any_of(entries.cbegin(), entries.cend(), [&net](const QString &entry) {
            return QFileInfo::exists(net + entry + QStringLiteral("/wireless"))
                || QFileInfo::exists(net + entry + QStringLiteral("/phy80211"));
        });
    }
    if (requirement == QLatin1String("bluetooth"))
        return !subfolders(sys + QStringLiteral("/class/bluetooth")).isEmpty();
    if (requirement == QLatin1String("touchpad"))
        return anInputIsNamed(sys, QStringLiteral("touchpad|trackpad|glidepoint"));
    if (requirement == QLatin1String("tablet"))
        return anInputIsNamed(sys, QStringLiteral("tablet|pen|stylus|wacom"));
    // the services Email & accounts can add, one file each
    if (requirement == QLatin1String("online-accounts"))
        return !QDir(root + QStringLiteral("/usr/share/accounts/providers"))
                    .entryList({QStringLiteral("*.provider")}, QDir::Files)
                    .isEmpty();
    // the account pages are a preview, behind a switch
    if (requirement == QLatin1String("raye-account"))
        return qEnvironmentVariable("RAYE_ACCOUNT_PREVIEW") == QLatin1String("1")
            || QFileInfo::exists(root + QStringLiteral("/etc/raye/account-preview"));
    // what Settings' Device security page looks for
    if (requirement == QLatin1String("firewall"))
        return QFileInfo::exists(root + QStringLiteral("/etc/ufw/ufw.conf"));
    return true;
}

bool Launcher::has(const QString &requirement) const
{
    if (requirement.isEmpty())
        return true;
    auto it = m_has.constFind(requirement);
    if (it == m_has.constEnd())
        it = m_has.insert(requirement, probe(m_root, requirement));
    return *it;
}

bool Launcher::hasPage(const QString &page) const
{
    if (!m_known)
        return true;
    return m_pages.contains(page) && has(m_needs.value(page));
}

void Launcher::openPage(const QString &page)
{
    start(QStringLiteral("raye-settings"), settingsArguments(page), QStringLiteral("org.raye.settings"));
}

bool Launcher::hasProgram(const QString &program) const
{
    auto it = m_programs.constFind(program);
    if (it == m_programs.constEnd())
        it = m_programs.insert(program, !QStandardPaths::findExecutable(program).isEmpty());
    return *it;
}

QString Launcher::firstProgram(const QStringList &programs) const
{
    for (const QString &program : programs) {
        if (hasProgram(program))
            return program;
    }
    return {};
}

void Launcher::run(const QString &program, const QStringList &arguments)
{
    if (!program.isEmpty())
        start(program, arguments, desktopNameFor(program));
}

QString Launcher::desktopNameFor(const QString &program)
{
    if (const KService::Ptr same = KService::serviceByDesktopName(program))
        return same->desktopEntryName();
    const KService::List found = KApplicationTrader::query([&program](const KService::Ptr &service) {
        const QString exec = service->exec().section(QLatin1Char(' '), 0, 0);
        return QFileInfo(exec).fileName() == program;
    });
    return found.isEmpty() ? QString() : found.constFirst()->desktopEntryName();
}

// Through KIO, as Plasma starts programs: detached, and with the startup
// feedback KWin shows (the busy pointer, Raye's ring) until the window comes.
void Launcher::start(const QString &program, const QStringList &arguments, const QString &desktopName)
{
    auto *job = new KIO::CommandLauncherJob(program, arguments, this);
    if (!desktopName.isEmpty())
        job->setDesktopName(desktopName);
    job->start();
    setBusy(true);
}

void Launcher::setBusyTimeout(int ms)
{
    m_busyTimer.setInterval(ms);
    Q_EMIT busyChanged();
}

void Launcher::setBusy(bool busy)
{
    if (busy)
        m_busyTimer.start();
    else
        m_busyTimer.stop();
    if (busy == m_busy)
        return;
    m_busy = busy;
    if (qGuiApp) {
        if (busy)
            QGuiApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
        else
            QGuiApplication::restoreOverrideCursor();
    }
    Q_EMIT busyChanged();
}
