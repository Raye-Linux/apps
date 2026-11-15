// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

// the name of a holiday of holidays.cpp, in the user's language
QString holidayName(const QString &id);
// "Canada (Quebec)", "France"...: the name of a region of Holidays::regions()
QString holidayRegionName(const QString &region);
