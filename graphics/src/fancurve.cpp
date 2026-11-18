// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fancurve.h"

#include <algorithm>
#include <cmath>

namespace Raye::Graphics
{

int speedAt(const QList<CurvePoint> &curve, double temp)
{
    if (curve.isEmpty())
        return 100;
    if (std::isnan(temp))
        return 100;
    if (temp <= curve.constFirst().temp)
        return curve.constFirst().speed;
    for (qsizetype i = 1; i < curve.size(); ++i) {
        const CurvePoint &a = curve.at(i - 1);
        const CurvePoint &b = curve.at(i);
        if (temp <= b.temp) {
            const double t = (temp - a.temp) / double(b.temp - a.temp);
            return int(std::lround(a.speed + t * (b.speed - a.speed)));
        }
    }
    return curve.constLast().speed;
}

int percentToPwm(int percent, int pwmMin, int pwmMax)
{
    percent = std::clamp(percent, 0, 100);
    return pwmMin + int(std::lround((pwmMax - pwmMin) * percent / 100.0));
}

FanController::FanController(const QList<CurvePoint> &curve, int fullSpeedAt)
    : m_curve(curve)
    , m_fullSpeedAt(fullSpeedAt)
{
}

int FanController::step(double temp)
{
    // no reading is treated as too hot: a sensor that stops answering
    // must never leave the fan slow
    const int wanted = (std::isnan(temp) || temp >= m_fullSpeedAt) ? 100 : speedAt(m_curve, temp);
    if (m_speed < 0 || wanted >= m_speed || std::isnan(temp) || temp <= m_setAt - hysteresis) {
        if (wanted != m_speed || m_speed < 0)
            m_setAt = std::isnan(temp) ? double(m_fullSpeedAt) : temp;
        m_speed = wanted;
    }
    return m_speed;
}

} // namespace Raye::Graphics
