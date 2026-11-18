# Clock

Alarms, timers, a stopwatch and world clocks, the way Windows' Clock
lays them out.

## Where things live

| What | Where |
|---|---|
| alarms | `~/.local/share/raye-clock/alarms.json` |
| timers | `~/.local/share/raye-clock/timers.json` |
| stopwatch and laps | `~/.local/share/raye-clock/stopwatch.json` |
| what rang, snoozes | `~/.local/state/raye-clock/rings.json` |
| world clocks, timer sound, window | `~/.config/raye/raye-clock.conf` |
| next ring | `~/.config/systemd/user/raye-clock-ring.timer` |

Each file is written whole, through a temporary file and a rename.

## Alarms ring with the window closed

1. Every change to an alarm or a timer runs `raye-clock-tool schedule`. It
   works out the next ring and writes one timer for that second (in UTC),
   or removes the timer when there is none.
2. At that second systemd starts `raye-clock-ring.service`, which runs
   `raye-clock-tool ring`: a notification with Snooze and Dismiss (a timer:
   Restart and Dismiss), and the alarm's sound in a loop, soft at first.
   The sound stops after ten minutes; the notification stays.
3. It writes the timer for the next ring and ends when the last ring on
   screen is answered.

A one-time alarm switches itself off once answered. A ring that came due
more than fifteen minutes ago (the computer slept or was off) shows as
"Missed alarm", without a sound. The timer is `Persistent=true`, so a ring
missed while the computer was off shows at sign-in.

Times are wall-clock times: an alarm at 7:00 rings at 7:00 in summer and
in winter. A time the clock skips when it moves forward rings when the
clock jumps; a time it passes twice rings the first time.

Timers and the stopwatch keep a moment (when the timer ends, when the
stopwatch started), so they run on with the window closed.

## What is not done

- An alarm cannot wake the computer from sleep: a systemd user timer
  cannot set the wake-up alarm, only the system manager can. The Alarm page
  says so.
- A change of time zone is seen at the next ring, or by the window when it
  is open or next opened.
- The sounds are made at build time by `sounds/make-sounds.py`.
