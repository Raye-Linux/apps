# xdg-desktop-portal-raye

The screen-sharing backend of the desktop portal. When an app (a browser,
Discord, OBS...) asks to share the screen, xdg-desktop-portal calls this
backend: it asks the user what to share, starts the streams through
KWin and PipeWire, and lists what is shared for the sharing indicator.

It serves `ScreenCast`, `RemoteDesktop` and the `Clipboard` of remote
desktop sessions (that portal works on the RemoteDesktop session, so it
must come from the same backend). `config/kde-portals.conf`, installed to
`/etc/xdg/xdg-desktop-portal/`, sends those portals here and every other
one to the desktop's own backend; each line ends with `kde`, so a portal
this backend does not offer still works.

## Where the code comes from

`src/` carries files of xdg-desktop-portal-kde (LGPL-2.0-or-later and
LGPL-2.1/3.0, see `LICENSES/`), listed in `UPSTREAM` with the checksum of
the release they come from. Files marked `edited` carry Raye changes,
each marked `Raye:`; the others are byte for byte the release. Raye's own
code is in `src/raye/`. On each Plasma release:

    ./sync-upstream.sh --check          # as-is files are still as released
    ./sync-upstream.sh 6.6.7            # what the new release changes

then copy the changed `as-is` files, merge the `edited` ones by hand,
update `UPSTREAM` (version, sha256 lines) and rebuild. Restore tokens keep
the `KDE` tag of the desktop's backend, so apps that saved a choice with it
(OBS) keep it here.

The upstream words keep the `xdg-desktop-portal-kde` catalog (installed by
that package, which this one depends on). Raye's words are in
`po/<lang>/*.po`.

## For the share picker (`qml/`)

A folder `qml/` with a `CMakeLists.txt` is built in: it must make a static
QML module, target `raye-portal-picker`, URI `org.raye.portal.picker`,
with a root component `SharePicker` that is a `Window`. It is shown instead
of the desktop's chooser (the user can go back with `Style=kde` in group
`[Picker]` of `~/.config/xdg-desktop-portal-rayerc`). If it does not load,
the desktop's chooser is shown.

Properties set when it is created:

| Property | Type | What |
|---|---|---|
| `title`, `mainText` | string | the desktop chooser's texts |
| `appId` | string | the portal's app id (may be empty) |
| `appName`, `appIconName` | string | from the app's `.desktop`; "An app" and a generic icon when unknown |
| `appKnown` | bool | whether the app was found |
| `outputsModel` | model or null | screens; also Entire workspace, Area and a virtual screen when offered |
| `windowsModel` | model or null | windows (roles of KWayland's PlasmaWindowModel, `checked` to select) |
| `multiple` | bool | the app takes several sources |
| `sourceTypes` | uint | what the app asked: 1 screens, 2 windows, 4 virtual |
| `cursorMode` | uint | the app's pointer mode: 1 hidden, 2 drawn in the video, 4 sent apart |
| `persistRequested` | bool | the app asked to remember the choice |

What it gives back: signals `accept()` and `reject()`, a function or slot
`present()`, a signal `clearSelection()`, and the properties read on
accept: `outputsModel`, `windowsModel` (selection is the `checked` role,
`Qt.Checked`), `allowRestore` (bool, "Remember this choice") and, when
`cursorMode` is not 1, `showPointer` (bool, "Share the pointer"; off sends
no pointer). Its words use the `xdg-desktop-portal-raye` catalog.

## For the sharing indicator (`indicator/`)

The list of what is shared is on the session bus: name
`org.raye.ScreenShare`, object `/org/raye/ScreenShare`, interface
`org.raye.ScreenShare` (`src/raye/sharesessions.h` has the fields):
property `Sessions` (`aa{sv}`), signal `SessionsChanged`, methods
`Stop(handle)` and `StopAll()`. Each session also has the desktop's own
tray item; an `indicator/` that draws its own defines
`RAYE_PORTAL_OWN_INDICATOR` on the `xdg-desktop-portal-raye` target to
hide it.

## For the permission prompts (`access/`)

A folder `access/` with a `CMakeLists.txt` adds its sources to the
`xdg-desktop-portal-raye` target and provides `access/rayeaccessportal.h`
with `class RayeAccessPortal : public QDBusAbstractAdaptor`, constructed
with the portal object as parent. `raye.portal` then lists
`org.freedesktop.impl.portal.Access`.

## Tests

`tests/`: the list of shared sessions (also on a private session bus), the
app name and icon, the picker choice, the installed files (portal, D-Bus,
systemd, KWin's interface list, `kde-portals.conf`, notifications) and
`sync-upstream.sh` on two small fake releases.
