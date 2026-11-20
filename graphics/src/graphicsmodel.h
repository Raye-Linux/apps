// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// What the window shows: the cards, the readings of the one chosen with
// the last minute of each, its limits and saved settings, and the root
// helper that applies new ones.

#pragma once

#include "cards.h"
#include "nvidiasmi.h"
#include "tuning.h"

#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class GraphicsModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QVariantList cards READ cards NOTIFY cardsChanged)
    Q_PROPERTY(int current READ current WRITE setCurrent NOTIFY cardChanged)
    Q_PROPERTY(QVariantMap card READ card NOTIFY cardChanged)
    Q_PROPERTY(QVariantMap now READ now NOTIFY readingChanged)
    Q_PROPERTY(QVariantMap history READ history NOTIFY readingChanged)
    Q_PROPERTY(QVariantMap limits READ limits NOTIFY limitsChanged)
    Q_PROPERTY(QVariantMap saved READ saved NOTIFY limitsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool nvidiaPanel READ nvidiaPanel CONSTANT)
    Q_PROPERTY(bool nvidiaReadings READ nvidiaReadings NOTIFY readingChanged)

public:
    explicit GraphicsModel(QObject *parent = nullptr);

    bool active() const { return m_active; }
    void setActive(bool active);
    QVariantList cards() const;
    int current() const { return m_current; }
    void setCurrent(int index);
    QVariantMap card() const;
    QVariantMap now() const { return m_now; }
    QVariantMap history() const;
    QVariantMap limits() const;
    QVariantMap saved() const;
    bool busy() const { return m_helper != nullptr; }
    bool nvidiaPanel() const;
    bool nvidiaReadings() const { return m_nvidiaSeen; }

    // whether the root helper would take these settings
    Q_INVOKABLE bool acceptable(const QVariantMap &settings) const;
    Q_INVOKABLE void apply(const QVariantMap &settings);
    Q_INVOKABLE void restoreDefaults();
    Q_INVOKABLE void openNvidiaPanel();
    // the fan speed a curve gives at a temperature, for the curve editor
    Q_INVOKABLE int speedAt(const QVariantList &curve, double temp) const;

Q_SIGNALS:
    void activeChanged();
    void cardsChanged();
    void cardChanged();
    void readingChanged();
    void limitsChanged();
    void busyChanged();
    // after apply or restoreDefaults: ok, and a sentence for the user
    void finished(bool ok, const QString &message);

private:
    void refresh();
    void readLimits();
    void readNvidia();
    void stopNvidia();
    void showNvidia(const Raye::Graphics::NvidiaReading &r);
    void push(const char *key, double value);
    void runHelper(const QStringList &args, const QByteArray &input, bool reset);
    const Raye::Graphics::Card *currentCard() const;

    QString m_sysRoot;
    QString m_configDir;
    QString m_nvidiaSmi;
    QList<Raye::Graphics::Card> m_cards;
    int m_current = 0;
    bool m_active = false;
    QTimer m_timer;
    QVariantMap m_now;
    QHash<QString, QVariantList> m_history;
    Raye::Graphics::Limits m_limits;
    Raye::Graphics::Config m_saved;
    QPointer<QProcess> m_helper;
    QPointer<QProcess> m_smi;
    bool m_nvidiaSeen = false;
};
