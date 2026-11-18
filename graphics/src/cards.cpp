// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cards.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>

namespace Raye::Graphics
{

QByteArray readFile(const QString &path, qint64 max)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.read(max);
}

QString readLine(const QString &path)
{
    return QString::fromUtf8(readFile(path, 4096)).trimmed();
}

qint64 readNumber(const QString &path, qint64 fallback)
{
    bool ok = false;
    const qint64 value = readLine(path).toLongLong(&ok);
    return ok ? value : fallback;
}

bool isSlot(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("\\A[0-9a-f]{4}:[0-9a-f]{2}:[0-9a-f]{2}\\.[0-7]\\z"));
    return re.match(text).hasMatch();
}

QString modelName(const QByteArray &pciIds, const QString &vendor, const QString &device)
{
    if (pciIds.isEmpty() || vendor.size() != 4 || device.size() != 4)
        return {};
    const QByteArray v = vendor.toLatin1();
    const QByteArray d = device.toLatin1();
    bool inVendor = false;
    for (const QByteArray &line : pciIds.split('\n')) {
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        if (!line.startsWith('\t')) {
            if (inVendor)
                break;
            inVendor = line.startsWith(QByteArray(v + "  "));
            continue;
        }
        if (!inVendor || line.startsWith("\t\t") || !line.mid(1).startsWith(QByteArray(d + "  ")))
            continue;
        QString name = QString::fromUtf8(line.mid(1 + d.size() + 2)).trimmed();
        // "Ellesmere [Radeon RX 580]": the part in brackets is what is on the box
        static const QRegularExpression box(QStringLiteral("\\[([^\\]]+)\\]\\s*$"));
        const auto m = box.match(name);
        if (m.hasMatch())
            name = m.captured(1);
        return name;
    }
    return {};
}

namespace
{
QString hexId(const QString &path)
{
    QString id = readLine(path).toLower();
    if (id.startsWith(QLatin1String("0x")))
        id = id.mid(2);
    return id;
}

Card readCard(const QString &drm, const QString &name, const QByteArray &pciIds)
{
    Card c;
    c.name = name;
    c.device = drm + QLatin1Char('/') + name + QStringLiteral("/device");
    c.vendorId = hexId(c.device + QStringLiteral("/vendor"));
    c.deviceId = hexId(c.device + QStringLiteral("/device"));
    if (c.vendorId == QLatin1String("1002"))
        c.vendor = Vendor::Amd;
    else if (c.vendorId == QLatin1String("10de"))
        c.vendor = Vendor::Nvidia;
    else if (c.vendorId == QLatin1String("8086"))
        c.vendor = Vendor::Intel;
    for (const QByteArray &line : readFile(c.device + QStringLiteral("/uevent")).split('\n')) {
        if (line.startsWith("DRIVER="))
            c.driver = QString::fromUtf8(line.mid(7)).trimmed();
        else if (line.startsWith("PCI_SLOT_NAME="))
            c.slot = QString::fromUtf8(line.mid(14)).trimmed().toLower();
    }
    if (c.slot.isEmpty())
        c.slot = QFileInfo(c.device).canonicalFilePath().section(QLatin1Char('/'), -1);
    const QDir hw(c.device + QStringLiteral("/hwmon"));
    const QStringList hwmons = hw.entryList({QStringLiteral("hwmon*")}, QDir::Dirs | QDir::System, QDir::Name);
    if (!hwmons.isEmpty())
        c.hwmon = hw.filePath(hwmons.constFirst());
    // product_name comes from the card's own firmware on recent cards
    c.model = readLine(c.device + QStringLiteral("/product_name"));
    if (c.model.isEmpty())
        c.model = modelName(pciIds, c.vendorId, c.deviceId);
    return c;
}
} // namespace

QList<Card> findCards(const QString &sysRoot, const QByteArray &pciIds)
{
    QList<Card> out;
    const QString drm = sysRoot + QStringLiteral("/class/drm");
    const QStringList names = QDir(drm).entryList({QStringLiteral("card*")}, QDir::Dirs | QDir::System, QDir::Name);
    for (const QString &name : names) {
        bool numeric = false;
        name.mid(4).toInt(&numeric);
        // card0-DP-1 and the like are the connectors of a card
        if (!numeric)
            continue;
        Card c = readCard(drm, name, pciIds);
        if (c.vendorId.isEmpty() || !isSlot(c.slot))
            continue;
        out.append(c);
    }
    // the window opens on the first: a card it can tune, then NVIDIA, then
    // the rest (a laptop's Intel chip comes after its NVIDIA card)
    const auto rank = [](const Card &c) {
        return c.tunable() ? 0 : c.vendor == Vendor::Nvidia ? 1 : c.vendor == Vendor::Amd ? 2 : 3;
    };
    std::stable_sort(out.begin(), out.end(), [&rank](const Card &a, const Card &b) {
        return rank(a) < rank(b);
    });
    return out;
}

Card cardAt(const QString &sysRoot, const QString &slot)
{
    if (!isSlot(slot))
        return {};
    const QList<Card> cards = findCards(sysRoot);
    for (const Card &c : cards) {
        if (c.slot == slot)
            return c;
    }
    return {};
}

} // namespace Raye::Graphics
