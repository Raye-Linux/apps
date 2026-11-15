# Calendar

A calendar of dates with reminders, and nothing else: no mail, no
accounts, no server.

## Where things live

| What | Where |
|---|---|
| events | `~/.local/share/raye-calendar/calendar.ics` (iCalendar, 0600) |
| reminders already shown | `~/.local/state/raye-calendar/reminders.json` |
| holidays, window choices | `~/.config/raye/raye-calendar.conf` |
| next reminder | `~/.config/systemd/user/raye-calendar-reminder.timer` |

The file is written whole, through a temporary file and a rename
(`CalStore::writeAtomic`): a full disk or a crash leaves the old file.

## Reminders, with nothing running in between

1. Each save runs `raye-calendar-tool schedule`. It works out the next
   reminder and writes one timer for that minute (in UTC), or removes the
   timer when there is none.
2. At that minute systemd starts `raye-calendar-reminder.service`, which
   runs `raye-calendar-tool fire`: a notification with Open, Later and
   Dismiss. Later comes back in five minutes, or five minutes before the
   start when that is further away.
3. It writes the timer for the next one and ends when the last reminder
   on screen is answered.

The timer is `Persistent=true`: a reminder that came due while the
computer was off shows at sign-in, if its event is not over yet.

## The clock on the taskbar

The clock applet (theme, `org.raye.clock`) runs `raye-calendar-tool
agenda FROM TO` when its calendar opens or changes month, marks the days
with events and lists the chosen day's, with "Add an event"
(`raye-calendar:?new=YYYY-MM-DD`).

## What is not done

- Only one reminder per event is edited here (others are kept in the file).
- Repeat rules beyond "every day/week/month/year" are kept and shown as
  read, but the editor offers the simple ones only.
- No VTIMEZONE is written: times carry the zone's name (TZID), which
  every calendar program reads.
