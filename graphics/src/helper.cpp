// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// raye-graphics-helper: the part of the graphics panel that runs as root.
//
//   apply <slot>   settings on stdin, through pkexec (org.raye.graphics.apply)
//   reset <slot>   the card's own settings back, through pkexec
//   boot           the saved settings of every card, at start (raye-graphics.service)
//   fan            follows the fan curves the cards cannot follow themselves
//                  (raye-graphics-fan.service)
//   fan-auto       gives those fans back to the card
//
// It reads the card's limits again and checks every value before writing,
// and only ever writes the files tuning.cpp names, under the card's own
// folder in /sys.

#include "cards.h"
#include "fancurve.h"
#include "tuning.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QThread>

#include <csignal>
#include <cstdio>
#include <unistd.h>

using namespace Raye::Graphics;

namespace
{
const QString sysRoot = QStringLiteral("/sys");
const QString configDir = QStringLiteral("/etc/raye/graphics");
const QString stateDir = QStringLiteral("/var/lib/raye/graphics");

enum Exit { Ok = 0, Usage = 2, NoCard = 3, Refused = 4, WriteFailed = 5 };

volatile std::sig_atomic_t stopping = 0;
void onSignal(int)
{
    stopping = 1;
}

void say(const QString &text)
{
    std::fprintf(stderr, "raye-graphics-helper: %s\n", qPrintable(text));
}

QString configFile(const QString &slot)
{
    return configDir + QLatin1Char('/') + slot + QStringLiteral(".conf");
}

bool writeSys(const Write &w)
{
    // the card's files are in /sys/devices; a link pointing anywhere else is refused
    const QString real = QFileInfo(w.file).canonicalFilePath();
    if (!real.startsWith(QLatin1String("/sys/devices/"))) {
        say(QStringLiteral("%1 is not a card file").arg(w.file));
        return false;
    }
    QFile f(real);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Unbuffered | QIODevice::ExistingOnly)) {
        say(QStringLiteral("cannot open %1").arg(real));
        return false;
    }
    if (f.write(w.value + '\n') < 0) {
        say(QStringLiteral("%1 refused %2").arg(real, QString::fromLatin1(w.value)));
        return false;
    }
    return true;
}

bool run(const QList<Write> &plan)
{
    for (const Write &w : plan)
        if (!writeSys(w))
            return false;
    return true;
}

bool saveFile(const QString &path, const QByteArray &text)
{
    QDir().mkpath(QFileInfo(path).path());
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(text);
    f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    return f.commit();
}

// the power limit before Raye first changed it, for cards that do not say
// what their default is
int savedCap(const QString &slot)
{
    const auto c = parseConfig(readFile(stateDir + QLatin1Char('/') + slot + QStringLiteral(".orig"), MaxConfigSize));
    return c ? c->powerCapW : 0;
}

void keepOriginal(const QString &slot, const Limits &l)
{
    const QString path = stateDir + QLatin1Char('/') + slot + QStringLiteral(".orig");
    if (!l.hasPowerCap || l.capDefaultW > 0 || QFile::exists(path))
        return;
    Config c;
    c.powerCapW = l.capW;
    saveFile(path, writeConfig(c));
}

void restartFanService()
{
    QProcess::execute(QStringLiteral("systemctl"), {QStringLiteral("restart"), QStringLiteral("raye-graphics-fan.service")});
}

Card tunableCard(const QString &slot)
{
    const Card card = cardAt(sysRoot, slot);
    if (card.name.isEmpty() || !card.tunable())
        return {};
    return card;
}

int reset(const QString &slot)
{
    const Card card = tunableCard(slot);
    if (card.name.isEmpty())
        return NoCard;
    QFile::remove(configFile(slot));
    const bool ok = run(planReset(card, readLimits(card), savedCap(slot)));
    restartFanService();
    return ok ? Ok : WriteFailed;
}

int apply(const QString &slot)
{
    const Card card = tunableCard(slot);
    if (card.name.isEmpty())
        return NoCard;
    QFile in;
    if (!in.open(stdin, QIODevice::ReadOnly))
        return Usage;
    QString why;
    const auto config = parseConfig(in.read(MaxConfigSize + 1), &why);
    if (!config) {
        say(why);
        return Refused;
    }
    const Limits limits = readLimits(card);
    why = checkConfig(*config, limits);
    if (!why.isEmpty()) {
        say(why);
        return Refused;
    }
    if (config->isDefault())
        return reset(slot);
    keepOriginal(slot, limits);
    if (!saveFile(configFile(slot), writeConfig(*config))) {
        say(QStringLiteral("cannot save %1").arg(configFile(slot)));
        return WriteFailed;
    }
    const bool ok = run(planApply(card, limits, *config));
    restartFanService();
    return ok ? Ok : WriteFailed;
}

struct Saved {
    Card card;
    Limits limits;
    Config config;
};

// the saved settings that still fit their card
QList<Saved> savedSettings()
{
    QList<Saved> out;
    const QStringList files = QDir(configDir).entryList({QStringLiteral("*.conf")}, QDir::Files, QDir::Name);
    for (const QString &name : files) {
        const QString slot = name.chopped(5);
        const Card card = tunableCard(slot);
        if (card.name.isEmpty()) {
            say(QStringLiteral("no card at %1, its settings wait").arg(slot));
            continue;
        }
        QString why;
        const auto config = parseConfig(readFile(configDir + QLatin1Char('/') + name, MaxConfigSize + 1), &why);
        const Limits limits = readLimits(card);
        if (config)
            why = checkConfig(*config, limits);
        if (!config || !why.isEmpty()) {
            say(QStringLiteral("%1 left alone: %2").arg(name, why));
            continue;
        }
        out.append({card, limits, *config});
    }
    return out;
}

int boot()
{
    // amdgpu may still be setting a card up when this starts
    const int wanted = int(QDir(configDir).entryList({QStringLiteral("*.conf")}, QDir::Files).size());
    QList<Saved> saved = savedSettings();
    for (int i = 0; i < 20 && saved.size() < wanted; ++i) {
        QThread::msleep(500);
        saved = savedSettings();
    }
    int status = Ok;
    for (const Saved &s : std::as_const(saved))
        if (!run(planApply(s.card, s.limits, s.config)))
            status = WriteFailed;
    return status;
}

void giveBack(const Card &card)
{
    writeSys({card.hwmon + QStringLiteral("/pwm1_enable"), "2"});
}

int fanAuto()
{
    for (const Saved &s : savedSettings())
        if (needsFanService(s.limits, s.config))
            giveBack(s.card);
    return Ok;
}

int fan()
{
    struct Follow {
        Card card;
        Limits limits;
        FanController controller;
        int failures = 0;
    };
    QList<Follow> cards;
    for (const Saved &s : savedSettings())
        if (needsFanService(s.limits, s.config))
            cards.append({s.card, s.limits, FanController(s.config.curve, s.limits.curveTempMax())});
    if (cards.isEmpty())
        return Ok;

    std::signal(SIGTERM, onSignal);
    std::signal(SIGINT, onSignal);
    while (!stopping && !cards.isEmpty()) {
        for (qsizetype i = cards.size() - 1; i >= 0; --i) {
            Follow &f = cards[i];
            const int speed = f.controller.step(curveTemperature(readReading(f.card)));
            const QString hw = f.card.hwmon + QLatin1Char('/');
            bool ok = true;
            // amdgpu hands the fan back to the card after a suspend
            if (readNumber(hw + QStringLiteral("pwm1_enable")) != 1)
                ok = writeSys({hw + QStringLiteral("pwm1_enable"), "1"});
            ok = ok && writeSys({hw + QStringLiteral("pwm1"),
                                 QByteArray::number(percentToPwm(speed, f.limits.pwmMin, f.limits.pwmMax))});
            f.failures = ok ? 0 : f.failures + 1;
            if (f.failures >= 3) {
                say(QStringLiteral("the fan of %1 does not answer, the card drives it again").arg(f.card.slot));
                giveBack(f.card);
                cards.removeAt(i);
            }
        }
        for (int t = 0; t < 20 && !stopping; ++t)
            ::usleep(100000);
    }
    for (const Follow &f : std::as_const(cards))
        giveBack(f.card);
    return Ok;
}
} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (::geteuid() != 0) {
        say(QStringLiteral("run through pkexec or systemd"));
        return Usage;
    }
    const QStringList a = app.arguments().mid(1);
    const QString verb = a.value(0);
    if (a.size() == 2 && (verb == QLatin1String("apply") || verb == QLatin1String("reset"))) {
        if (!isSlot(a.at(1)))
            return Usage;
        return verb == QLatin1String("apply") ? apply(a.at(1)) : reset(a.at(1));
    }
    if (a.size() == 1 && verb == QLatin1String("boot"))
        return boot();
    if (a.size() == 1 && verb == QLatin1String("fan"))
        return fan();
    if (a.size() == 1 && verb == QLatin1String("fan-auto"))
        return fanAuto();
    say(QStringLiteral("apply <slot>, reset <slot>, boot, fan or fan-auto"));
    return Usage;
}
