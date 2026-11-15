# raye-taskmanager

The Task Manager: what is using the machine, and the two things a user
comes here to do -- end a program that has stopped answering, and hold
one back so the rest of the machine keeps up.

Started as `raye-taskmanager`; its desktop file is
`org.raye.taskmanager.desktop`, which is the name the Raye shell asks
for (taskbar menu, clock menu, desktop menu, and the Run box, where
`taskmgr`, `resmon` and `perfmon` all map to `org.raye.taskmanager` --
see `theme/shell/run/src/runparser.cpp`).

## The pages

A navigation pane on the left, as in Windows 11, the search box along the
top (it filters Processes, Startup apps, Details and Services), and these
pages:

| Page | What fills it |
|---|---|
| **Processes** | `/proc/<pid>/stat` for the name, state, processor and memory, `/proc/<pid>/io` for the disk, `/proc/<pid>/cgroup` for the app a process belongs to. Apps first, each with its processes under it; then background processes. The headings carry the machine's totals |
| **Performance** | `/proc/stat`, `/proc/meminfo`, `/proc/diskstats`, `/proc/net/dev`, `/proc/cpuinfo`, `/sys/devices/system/cpu` (caches, clock), `/sys/block`, `/sys/class/net`, `/sys/class/drm` and `pci.ids` for the graphics card. A minute of history per device |
| **Startup apps** | the XDG autostart folders; see below |
| **Users** | the processes of each account with a uid of 1000 or more, rolled up |
| **Details** | every process, kernel threads included, with its number, owner, threads and priority; end task, end process tree, set priority, efficiency mode |
| **Services** | systemd over D-Bus (`ListUnits`, `ListUnitFiles`); start, stop and restart go through polkit |
| Settings | the start page and the update speed, in `~/.config/raye/raye-taskmanager.conf` |

**App history is not a page.** Linux keeps no per-app total across
sessions: cgroup v2 gives `cpu.stat` and `memory.current` for a unit, and
they are reset when the unit stops. This app would have to keep the tally
itself, which is a design decision, not a page; an empty page where
Windows shows a month of figures reads as a fault in the machine.

**Grouping.** The shell starts every app in a scope of its own,
`app-<desktop id>-<random>.scope` (`appgroups.h`); everything the app
starts lands in it, so a terminal and its shells, a browser and its
tabs, are one row. Apps started at sign-in run as
`app-<id>@autostart.service` and are listed with the background processes.
The kernel's own threads (`PF_KTHREAD`, field 9) are only in Details.

**Cost.** One reading a second (two at "High", one every four at "Low"),
one every five seconds while the window is minimised. Lists are not bound
while their page is hidden, and a reading where a process started or ended
inserts and removes rows rather than resetting the list (`rowdiff.h`):
measured under Xvfb with 440 processes, 3.8 % of one core on Processes and
4.6 % on Details, against 15.9 % and 15.7 % with a reset each second. No
service runs while the window is closed. A second Ctrl+Shift+Esc brings
the open window forward (`KDBusService::Unique`).

## Startup apps

The entries live in `/etc/xdg/autostart` (the system's) and in
`~/.config/autostart` (the person's). A file of the same name in the
person's folder replaces the system one, so **turning a system entry off
writes a copy of it under the home with `Hidden=true`**; the system file
is never touched, and turning it back on removes the key again
(`autostart.h`; `tests/test_autostart.cpp` checks the system file's hash).

"Startup impact" has no equivalent on Linux. The figure shown is the
processor time the entry's unit has used since sign-in, read from its
cgroup's `cpu.stat`, in Windows' bands: under 300 ms Low, under a second
Medium, above High. An entry that is not running says "Not measured".

## Services

Starting, stopping and restarting call `StartUnit`, `StopUnit` and
`RestartUnit` on systemd's manager directly, never `systemctl`, with
interactive authorisation allowed: polkit then asks for an administrator's
password (`org.freedesktop.systemd1.manage-units` is `auth_admin_keep`
for an active session), and a refusal or a cancelled prompt comes back as
an error the page shows. The PID is the first process of the unit's
cgroup; `MainPID` would cost one D-Bus call per unit and per refresh.

## Priority

Windows' five classes map onto nice values: Low 19, Below normal 10,
Normal 0, Above normal -5, High -15. Real-time is left out. Lowering the
priority of one's own process is done in place; raising one, or touching
another user's, goes through `pkexec renice`, which shows its own prompt.

## The columns that are deliberately absent

**Network, per process.** The kernel has no per-process byte count at
all. And the file that looks like one is a trap: `/proc/<pid>/net/dev`
exists, but it belongs to the **network namespace**, so every process in
the same namespace shows the same totals -- the whole machine's. A real
per-process figure needs eBPF or a packet capture (what `nethogs` does),
which is a privileged helper, not a column. The machine's network is on
the Performance page.

**Disk** is shown for the user's own processes; `/proc/<pid>/io` of
another user's process is `Permission denied`, and that row shows a dash
rather than a 0 that was never measured.

**GPU, per process**, needs the DRM fdinfo of every process, which only
some drivers fill. The card's own load is on the Performance page when
its driver gives one (`gpu_busy_percent`), and the page says so when not.

## Efficiency mode: measured, and one-way

Two busy loops pinned to one processor for three seconds, jiffies each:

| | ordinary | treated |
|---|---|---|
| ordinary against ordinary | 149 | 150 |
| ordinary against `SCHED_IDLE` | 298 | **1** |
| ordinary against `nice 19` | 295 | 4 |
| ordinary against both | 298 | 1 |

So the idle class is the real lever and `nice` is nearly nothing beside
it, and **no weight can buy a `SCHED_IDLE` process a share back**: the
class is consulted before any weight. A process in this mode gets the
processor nobody else wants, and runs at full speed when nobody wants
it. That is what the window says, in those words.

**And it cannot be undone by the person who did it.** A thread may
always lower its own kind's priority and never raise it back. Measured:
an ordinary user switching one of their own processes out of
`SCHED_IDLE` gets `EPERM`, and so does `renice` from 19 back to 0. The
one thing that changes it is `RLIMIT_NICE`, and the boundary was
measured one unit at a time:

| `RLIMIT_NICE` | leaving `SCHED_IDLE` |
|---|---|
| 0 (the Debian default) | refused |
| 18 | refused |
| 19 | refused |
| **20** | **allowed** |
| 21 | allowed |

`CAP_SYS_NICE` settles it whatever the limit. The I/O half is the
exception: the idle I/O class goes on and comes off freely.

The window therefore **asks before it puts a process in**, and when the
kernel will not allow the way back it says so in the question instead of
drawing a switch that fails when pressed. `ProcessModel.efficiencyIsReversible`
is that answer, read from the kernel at startup.

## End task: why two signals

`SIGTERM` is the request an editor answers by saving. `SIGKILL` is not
answered at all, so a program forced that way loses what it had not
written. The window asks, waits `EndTaskGraceMs` (5 s, about what
Windows waits before it calls a program unresponsive), and only then
offers the force -- which still takes a second press.

**Every signal is checked against the process's start time**, field 22
of `/proc/<pid>/stat`. Process numbers are handed out up to
`/proc/sys/kernel/pid_max` and then wrap round, so between the moment a
row is drawn and the moment its button is pressed that number may belong
to something else. The number alone is not an identity; the number and
the start time together are. A stale row ends nothing rather than ending
the wrong thing. The same guard is on the efficiency mode: slowing the
wrong process to a crawl looks exactly like a broken machine.

## The trap in `/proc/<pid>/stat`

The name is printed between brackets and is **not escaped**. A program
called `evil) S 1 1 1 0 -1 0` produces a line that, read from the first
`)`, is a different process with a state and a parent of its
choosing. The only safe landmark is the **last** `)`: nothing follows the
closing bracket but numbers. `parseProcStat()` uses it, and
`tests/test_proctable.cpp` proves it -- reading from the first bracket
turns that test red.

## What is left

- **Memory is RSS**, so pages shared between two processes are counted in
  both. The figure nearer to Windows' is `Pss`, from
  `/proc/<pid>/smaps_rollup`, which walks the whole address space per
  process; it belongs on the selected row in Details.
- **Users** offers no "Sign out": logind's `TerminateUser` is there to
  call, the question to ask first is not written.
- **Speed: Paused** still reads every five seconds rather than stopping.

## Style

The glyphs of the navigation and the command bar are drawn in
`Glyph.qml`, on a 16 px grid; the drop-down, the check box and the
right-click menu are the window's own (`ChoiceButton`, `CheckRow`,
`ContextMenu`), so nothing depends on which Qt Quick Controls style the
session sets.

Every colour, radius, spacing, type size and duration comes from
`Tokens` (`import org.raye.ui`), so the window follows the user's accent
and the Night/Day scheme with no colour of its own, like the share
picker in `portal/`. The type tokens are **fonts**, not sizes:
`font: Tokens.body`, never `font.pixelSize: Tokens.body` --
`qmllint` catches that one, and did.

## Build and test

Part of `raye-apps`; see the repository README. The logic is a library
of its own that links `Qt::Core` alone, so its tests need no display, no
session bus and no translation library:

    ctest --test-dir build -R 'raye-taskmanager/'

Test programs: `proctable`, `endtask`, `efficiency`, `sysinfo`,
`appgroups`, `autostart`, `services`, `rowdiff`; and two scripts,
`test_columns.py` (the Processes widths) and `test_french.py` (every word
the window shows has its French, read with xgettext).
`tests/CMakeLists.txt` must name every file in `tests/`;
`cmake/tests/test_registration.cmake` fails the build when one is
missing, because an unregistered test is never compiled and nothing else
says so.

## Packaging

`debian/control` listed `qml6-module-org-kde-ksysguard` and
`libksysguard-bin` while the folder was empty. Neither is used: the app
reads `/proc` itself, which is what lets the logic be a Qt-only library
with tests. They were dropped rather than left to pull a library onto
every disk for nothing. If the Performance tab later wants ksysguard's
sensor model, that is the moment to put them back.
