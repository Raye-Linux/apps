# Graphics Card

`raye-graphics`, the graphics card panel: live readings of the card, and
on AMD Radeon cards the settings amdgpu lets a user change. NVIDIA cards
show their readings (from `nvidia-smi`) and open the NVIDIA Control
Panel; Intel and other cards get a plain sentence saying there is
nothing to change.

## Pages

- **Overview**: GPU use, clock speed, video memory (shared memory on an
  integrated GPU), temperature with hotspot and memory, power with its
  limit, fan, and what the card is. A reading the card does not give is
  not shown.
- **Performance**: `power_dpm_force_performance_level` (auto, low,
  high), the workload profile of `pp_power_profile_mode` (CUSTOM is not
  offered), and `hwmon/power1_cap` between the card's limits.
- **Fan**: the card's own control, or a curve. RDNA3 and later follow
  the curve themselves (`gpu_od/fan_ctrl/fan_curve`, written point by
  point then committed); older cards get `raye-graphics-fan.service`,
  which sets `pwm1` every two seconds from the junction sensor (the edge
  one if there is none).

## What is written, and who writes it

The window never writes to sysfs. Apply sends the settings, as a few
`key=value` lines, to `/usr/libexec/raye-graphics-helper` through pkexec
(`org.raye.graphics.apply`). The helper:

- takes the card by its PCI slot (`0000:03:00.0`, checked by pattern),
  and only an amdgpu card;
- reads the card's limits again and runs the same checks as the window
  (`tuning.cpp`); a value outside the card's range is refused before any
  write;
- writes only the files `planApply` names, all under that card's folder,
  and refuses a path that resolves outside `/sys/devices`;
- saves the settings in `/etc/raye/graphics/<slot>.conf`, which
  `raye-graphics.service` applies again at each start.

The lowest power limit offered is 40 % of the top one: many cards say
0 W, and a card held that low makes the whole desktop crawl. Above the
top point of a curve the fan service runs the fan flat out, and a sensor
that stops answering counts as hot. When the service stops, the card
gets its fan back.

Restore defaults writes the card's own values back (its published default
power limit, or the one kept in `/var/lib/raye/graphics/<slot>.orig`
before the first change) and removes the saved settings.

## Tests

`tests/fakecards.h` writes the sysfs files of an RX 580, an RX 6800 XT,
an RX 7900 XTX and a Cezanne integrated GPU as amdgpu shows them; the
tests read those, check the settings and compare the exact writes.
`RAYE_GRAPHICS_SYSFS` points the window at such a tree for screenshots.
