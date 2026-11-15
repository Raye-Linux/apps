# Raye apps

Raye Linux's own small apps, in one repo: Terminal, Notepad, Task Manager,
Snipping Tool, Calculator, Camera, Photos, Device Manager, Sticky Notes,
Game Bar, the screen sharing backend, the "not responding" window, and the
names and defaults given to the stock apps of the desktop.

One source package, `raye-apps`, makes one Debian package per app. Each app
has its own folder; `tests/` holds the checks that cover all of them.

## Build

    cmake -S . -B build -G Ninja -DBUILD_TESTING=ON
    cmake --build build

Needs Qt 6, KDE Frameworks 6 and the Plasma 6 development packages.

## Test

    ctest --test-dir build --output-on-failure

To build the packages: `dpkg-buildpackage -b -us -uc`.

## License

GPL-3.0-or-later (`LICENSE`). Some files carry LGPL-2.1-or-later or CC0,
as their header says; the texts are in `LICENSES/`.

## Links

- Website: https://rayelinux.com
- Discord: https://discord.gg/TrxhUkabyB (questions, bug reports, news)
