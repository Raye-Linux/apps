// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A fan curve: the fan speed for a temperature, between the points the
// user set. Cards before RDNA3 have no curve of their own, so the helper
// follows it, every two seconds, with FanController.

#pragma once

#include "amdgpu.h"

#include <QList>

namespace Raye::Graphics
{

// linear between two points, flat before the first and after the last
int speedAt(const QList<CurvePoint> &curve, double temp);

// percent to pwm1, in the card's own range
int percentToPwm(int percent, int pwmMin, int pwmMax);

class FanController
{
public:
    // above fullSpeedAt the fan runs at 100 % whatever the curve says
    FanController(const QList<CurvePoint> &curve, int fullSpeedAt);

    // the speed for a new reading. The speed goes up at once and comes
    // down only once the card has cooled by a few degrees, so the fan
    // does not hunt around one point of the curve.
    int step(double temp);
    int speed() const { return m_speed; }

    static constexpr double hysteresis = 3.0;

private:
    QList<CurvePoint> m_curve;
    int m_fullSpeedAt;
    int m_speed = -1;
    double m_setAt = 0; // the temperature that set m_speed
};

} // namespace Raye::Graphics
