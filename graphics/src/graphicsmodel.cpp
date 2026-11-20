// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "graphicsmodel.h"
#include "amdgpu.h"
#include "fancurve.h"

#include <KLocalizedString>

#include <QStandardPaths>

#include <cmath>

using namespace Raye::Graphics;

namespace
{
constexpr int historyLength = 60;

QString env(const char *name, const QString &fallback)
{
    const QString v = qEnvironmentVariable(name);
    return v.isEmpty() ? fallback : v;
}

QString vendorName(Vendor v)
{
    switch (v) {
    case Vendor::Amd:
        return QStringLiteral("amd");
    case Vendor::Nvidia:
        return QStringLiteral("nvidia");
    case Vendor::Intel:
        return QStringLiteral("intel");
    case Vendor::Other:
        break;
    }
    return QStringLiteral("other");
}

QVariant real(double v)
{
    return std::isnan(v) ? QVariant(-1) : QVariant(v);
}

QVariantList curveList(const QList<CurvePoint> &curve)
{
    QVariantList out;
    for (const CurvePoint &p : curve)
        out.append(QVariantMap{{QStringLiteral("temp"), p.temp}, {QStringLiteral("speed"), p.speed}});
    return out;
}

Config toConfig(const QVariantMap &m)
{
    Config c;
    c.performance = m.value(QStringLiteral("performance"), c.performance).toString();
    c.profile = m.value(QStringLiteral("profile"), -1).toInt();
    c.powerCapW = m.value(QStringLiteral("powerCapW"), 0).toInt();
    c.fanCurve = m.value(QStringLiteral("fanCurve"), false).toBool();
    for (const QVariant &v : m.value(QStringLiteral("curve")).toList()) {
        const QVariantMap p = v.toMap();
        c.curve.append({p.value(QStringLiteral("temp")).toInt(), p.value(QStringLiteral("speed")).toInt()});
    }
    return c;
}
} // namespace

GraphicsModel::GraphicsModel(QObject *parent)
    : QObject(parent)
    , m_sysRoot(env("RAYE_GRAPHICS_SYSFS", QStringLiteral("/sys")))
    , m_configDir(env("RAYE_GRAPHICS_CONFIG", QStringLiteral("/etc/raye/graphics")))
    , m_nvidiaSmi(env("RAYE_GRAPHICS_NVIDIA_SMI", QStringLiteral("nvidia-smi")))
{
    m_cards = findCards(m_sysRoot, readFile(QStringLiteral("/usr/share/misc/pci.ids"), 64 * 1024 * 1024));
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &GraphicsModel::refresh);
    readLimits();
    refresh();
}

void GraphicsModel::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    if (active) {
        refresh();
        m_timer.start();
    } else {
        m_timer.stop();
        stopNvidia();
    }
    Q_EMIT activeChanged();
}

QVariantList GraphicsModel::cards() const
{
    QVariantList out;
    for (const Card &c : m_cards)
        out.append(QVariantMap{{QStringLiteral("model"), c.model}, {QStringLiteral("vendor"), vendorName(c.vendor)},
                               {QStringLiteral("slot"), c.slot}});
    return out;
}

const Card *GraphicsModel::currentCard() const
{
    return m_current >= 0 && m_current < m_cards.size() ? &m_cards.at(m_current) : nullptr;
}

void GraphicsModel::setCurrent(int index)
{
    if (index == m_current || index < 0 || index >= m_cards.size())
        return;
    m_current = index;
    stopNvidia();
    m_history.clear();
    m_nvidiaSeen = false;
    readLimits();
    refresh();
    Q_EMIT cardChanged();
}

QVariantMap GraphicsModel::card() const
{
    const Card *c = currentCard();
    if (!c)
        return {};
    return {{QStringLiteral("model"), c->model},
            {QStringLiteral("vendor"), vendorName(c->vendor)},
            {QStringLiteral("driver"), c->driver},
            {QStringLiteral("slot"), c->slot},
            {QStringLiteral("tunable"), c->tunable()},
            {QStringLiteral("vbios"), readLine(c->device + QStringLiteral("/vbios_version"))}};
}

void GraphicsModel::push(const char *key, double value)
{
    QVariantList &list = m_history[QLatin1String(key)];
    list.append(std::isnan(value) || value < 0 ? 0.0 : value);
    while (list.size() > historyLength)
        list.removeFirst();
}

QVariantMap GraphicsModel::history() const
{
    QVariantMap out;
    for (auto it = m_history.cbegin(); it != m_history.cend(); ++it)
        out.insert(it.key(), it.value());
    return out;
}

void GraphicsModel::refresh()
{
    const Card *c = currentCard();
    if (!c)
        return;
    if (c->vendor == Vendor::Nvidia) {
        readNvidia();
        return;
    }
    const Reading r = readReading(*c);
    const double temp = std::isnan(r.edge) ? r.junction : r.edge;
    m_now = {{QStringLiteral("busy"), r.busy},
             {QStringLiteral("memBusy"), r.memBusy},
             {QStringLiteral("vramUsed"), double(r.vramUsed)},
             {QStringLiteral("vramTotal"), double(r.vramTotal)},
             {QStringLiteral("gttUsed"), double(r.gttUsed)},
             {QStringLiteral("gttTotal"), double(r.gttTotal)},
             {QStringLiteral("sclk"), r.sclk},
             {QStringLiteral("mclk"), r.mclk},
             {QStringLiteral("temp"), real(temp)},
             {QStringLiteral("junction"), real(r.junction)},
             {QStringLiteral("memTemp"), real(r.memory)},
             {QStringLiteral("curveTemp"), real(curveTemperature(r))},
             {QStringLiteral("fanRpm"), r.fanRpm},
             {QStringLiteral("fanPercent"), r.fanPercent},
             {QStringLiteral("powerW"), real(r.powerW)},
             {QStringLiteral("powerLimitW"), m_limits.hasPowerCap ? QVariant(m_limits.capW) : QVariant(-1)}};
    push("busy", r.busy);
    push("sclk", r.sclk);
    push("temp", temp);
    push("power", r.powerW);
    push("fan", r.fanPercent);
    push("vram", r.vramTotal > 0 ? 100.0 * double(r.vramUsed) / double(r.vramTotal) : -1);
    Q_EMIT readingChanged();
}

void GraphicsModel::readNvidia()
{
    // one nvidia-smi for as long as the window shows this card; it prints
    // a line a second (nvidiaSmiArguments)
    if (m_smi)
        return;
    const QString program = QStandardPaths::findExecutable(m_nvidiaSmi);
    if (program.isEmpty())
        return;
    m_smi = new QProcess(this);
    m_smi->setProcessChannelMode(QProcess::SeparateChannels);
    const QString slot = currentCard()->slot;
    int nvidiaCards = 0;
    for (const Card &c : std::as_const(m_cards))
        nvidiaCards += c.vendor == Vendor::Nvidia;
    connect(m_smi, &QProcess::readyReadStandardOutput, this, [this, slot, nvidiaCards] {
        while (m_smi && m_smi->canReadLine()) {
            const QByteArray line = m_smi->readLine();
            const Card *c = currentCard();
            if (!c || c->slot != slot)
                return;
            for (const NvidiaReading &r : parseNvidiaSmi(line)) {
                if (r.slot != slot && nvidiaCards > 1)
                    continue;
                showNvidia(r);
            }
        }
    });
    QProcess *smi = m_smi;
    auto gone = [this, smi] {
        if (m_smi == smi)
            m_smi = nullptr;
        smi->deleteLater();
    };
    connect(m_smi, &QProcess::finished, this, gone);
    connect(m_smi, &QProcess::errorOccurred, this, [gone](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart)
            gone();
    });
    m_smi->start(program, nvidiaSmiArguments());
}

void GraphicsModel::stopNvidia()
{
    if (!m_smi)
        return;
    QProcess *smi = m_smi;
    m_smi = nullptr;
    smi->disconnect(this);
    smi->terminate();
    if (!smi->waitForFinished(500))
        smi->kill();
    smi->deleteLater();
}

void GraphicsModel::showNvidia(const NvidiaReading &r)
{
    m_nvidiaSeen = true;
    m_now = {{QStringLiteral("name"), r.name},
             {QStringLiteral("driver"), r.driver},
             {QStringLiteral("busy"), r.busy},
             {QStringLiteral("memBusy"), r.memBusy},
             {QStringLiteral("vramUsed"), double(r.vramUsed)},
             {QStringLiteral("vramTotal"), double(r.vramTotal)},
             {QStringLiteral("gttUsed"), -1},
             {QStringLiteral("gttTotal"), -1},
             {QStringLiteral("sclk"), r.sclk},
             {QStringLiteral("mclk"), r.mclk},
             {QStringLiteral("temp"), real(r.temp)},
             {QStringLiteral("junction"), -1},
             {QStringLiteral("memTemp"), -1},
             {QStringLiteral("fanRpm"), -1},
             {QStringLiteral("fanPercent"), r.fanPercent},
             {QStringLiteral("powerW"), real(r.powerW)},
             {QStringLiteral("powerLimitW"), real(r.powerLimitW)}};
    push("busy", r.busy);
    push("sclk", r.sclk);
    push("temp", r.temp);
    push("power", r.powerW);
    push("fan", r.fanPercent);
    push("vram", r.vramTotal > 0 ? 100.0 * double(r.vramUsed) / double(r.vramTotal) : -1);
    Q_EMIT readingChanged();
}

void GraphicsModel::readLimits()
{
    const Card *c = currentCard();
    m_limits = (c && c->tunable()) ? Raye::Graphics::readLimits(*c) : Limits();
    m_saved = Config();
    if (c) {
        const auto saved = parseConfig(readFile(m_configDir + QLatin1Char('/') + c->slot + QStringLiteral(".conf"),
                                                MaxConfigSize + 1));
        if (saved && checkConfig(*saved, m_limits).isEmpty())
            m_saved = *saved;
    }
    if (m_saved.curve.isEmpty() && m_limits.canCurve())
        m_saved.curve = defaultCurve(m_limits);
    Q_EMIT limitsChanged();
}

QVariantMap GraphicsModel::limits() const
{
    QVariantList profiles;
    for (const PowerProfile &p : m_limits.profiles)
        if (p.name != QLatin1String("CUSTOM"))
            profiles.append(QVariantMap{{QStringLiteral("index"), p.index}, {QStringLiteral("name"), p.name},
                                        {QStringLiteral("active"), p.active}});
    const Limits &l = m_limits;
    return {{QStringLiteral("level"), l.level},
            {QStringLiteral("profiles"), profiles},
            {QStringLiteral("defaultProfile"), l.defaultProfile},
            {QStringLiteral("hasPowerCap"), l.hasPowerCap},
            {QStringLiteral("capW"), l.capW},
            {QStringLiteral("capMinW"), l.capMinW},
            {QStringLiteral("capMaxW"), l.capMaxW},
            {QStringLiteral("capDefaultW"), l.capDefaultW},
            {QStringLiteral("hasFan"), l.hasFan},
            {QStringLiteral("canCurve"), l.canCurve()},
            {QStringLiteral("firmwareCurve"), l.odCurve.valid},
            {QStringLiteral("junction"), l.junctionSensor},
            {QStringLiteral("curveTempMin"), l.curveTempMin()},
            {QStringLiteral("curveTempMax"), l.curveTempMax()},
            {QStringLiteral("curveSpeedMin"), l.curveSpeedMin()},
            {QStringLiteral("curveSpeedMax"), l.curveSpeedMax()},
            {QStringLiteral("curvePoints"), l.curvePoints()},
            {QStringLiteral("defaultCurve"), curveList(defaultCurve(l))}};
}

QVariantMap GraphicsModel::saved() const
{
    return {{QStringLiteral("performance"), m_saved.performance},
            {QStringLiteral("profile"), m_saved.profile},
            {QStringLiteral("powerCapW"), m_saved.powerCapW},
            {QStringLiteral("fanCurve"), m_saved.fanCurve},
            {QStringLiteral("curve"), curveList(m_saved.curve)},
            {QStringLiteral("isDefault"), m_saved.isDefault()}};
}

bool GraphicsModel::acceptable(const QVariantMap &settings) const
{
    const Card *c = currentCard();
    return c && c->tunable() && checkConfig(toConfig(settings), m_limits).isEmpty();
}

int GraphicsModel::speedAt(const QVariantList &curve, double temp) const
{
    QVariantMap m{{QStringLiteral("curve"), curve}};
    return Raye::Graphics::speedAt(toConfig(m).curve, temp);
}

bool GraphicsModel::nvidiaPanel() const
{
    return !QStandardPaths::findExecutable(QStringLiteral("nvidia-settings")).isEmpty();
}

void GraphicsModel::openNvidiaPanel()
{
    QProcess::startDetached(QStringLiteral("nvidia-settings"), {});
}

void GraphicsModel::apply(const QVariantMap &settings)
{
    const Card *c = currentCard();
    if (!c || busy() || !acceptable(settings))
        return;
    Config config = toConfig(settings);
    // a curve kept only for the editor is not a setting
    if (!config.fanCurve)
        config.curve.clear();
    runHelper({QStringLiteral("apply"), c->slot}, writeConfig(config), false);
}

void GraphicsModel::restoreDefaults()
{
    const Card *c = currentCard();
    if (!c || busy() || !c->tunable())
        return;
    runHelper({QStringLiteral("reset"), c->slot}, {}, true);
}

void GraphicsModel::runHelper(const QStringList &args, const QByteArray &input, bool reset)
{
    m_helper = new QProcess(this);
    connect(m_helper, &QProcess::finished, this, [this, reset](int code, QProcess::ExitStatus status) {
        m_helper->deleteLater();
        m_helper = nullptr;
        Q_EMIT busyChanged();
        readLimits();
        refresh();
        if (status != QProcess::NormalExit) {
            Q_EMIT finished(false, i18nc("@info", "The change was stopped before the end. Check the settings and try again."));
            return;
        }
        switch (code) {
        case 0:
            Q_EMIT finished(true, reset ? i18nc("@info", "The card is back on its own settings.")
                                        : i18nc("@info", "Your settings are on. They come back each time the PC starts."));
            return;
        case 126:
        case 127:
            // the password was not given: nothing changed, nothing to say
            Q_EMIT finished(false, QString());
            return;
        case 3:
            Q_EMIT finished(false, i18nc("@info", "This card can't be found anymore. Close the panel and open it again."));
            return;
        case 4:
            Q_EMIT finished(false, i18nc("@info", "The card doesn't accept these settings. Nothing was changed."));
            return;
        default:
            Q_EMIT finished(false, i18nc("@info", "The card refused part of the change. Use Restore defaults if it misbehaves."));
            return;
        }
    });
    connect(m_helper, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart || !m_helper)
            return;
        m_helper->deleteLater();
        m_helper = nullptr;
        Q_EMIT busyChanged();
        Q_EMIT finished(false, i18nc("@info", "Root access is needed to change the card, and it can't be asked for here."));
    });
    m_helper->start(QStringLiteral("pkexec"), QStringList{QStringLiteral(RAYE_GRAPHICS_HELPER)} + args);
    m_helper->write(input);
    m_helper->closeWriteChannel();
    Q_EMIT busyChanged();
}
