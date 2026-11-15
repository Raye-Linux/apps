// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// Glue between the screen-cast sessions and the list of what is shared
// (sharesessions.h). Called once streams run; the session leaves the
// list when it closes.

#pragma once

class ScreenCastSession;

namespace Raye
{
void publishShare(ScreenCastSession *session);
}
