# Registry Editor, Raye edition

`raye-regedit` shows the settings files of the system the way the Windows
Registry Editor shows the registry: a tree of keys on the left, the
values of the selected key on the right. It edits the real files, so
everything it shows is what the apps read.

Linux has no registry. What it has is a few hundred small settings
files and one binary database (dconf). This app gives them one tree and
one way to edit them, and takes care not to break any of them.

## The tree

| Root | What is under it | Writable |
|---|---|---|
| `HKEY_CURRENT_USER\Config` | the user's settings files in `~/.config`, one key per file, one subkey per group | yes, as the user |
| `HKEY_CURRENT_USER\Desktop` | the user's dconf database (GTK apps, Firefox, GNOME parts) | yes, as the user |
| `HKEY_LOCAL_MACHINE\Defaults` | `/etc/xdg`: what every new account starts with | yes, after an admin password |
| `HKEY_LOCAL_MACHINE\System` | a chosen list of `/etc` files (below) | yes, after an admin password |
| `HKEY_LOCAL_MACHINE\Information` | `/etc/os-release`, `/etc/hostname` | read only |

A file becomes a key under its name (`kdeglobals`, `kwinrc`,
`gtk-3.0\settings.ini`). A group becomes a subkey (`[General]` is
`kdeglobals\General`, `[Containments][1][General]` is three levels). Keys
before the first group are the values of the file's own key, and so are
all the values of a shell file such as `/etc/default/grub`.

### What is shown from `~/.config`

Every file up to three folders deep that is under 512 KiB and reads as a
settings file: its first line that is not blank and not a comment is a
`[group]` or a `key=value`. JSON, XML, databases and binary files are left
out, so is `~/.config/dconf` (it is the dconf root), and so are the
folders of the browsers, which hold their own databases.

### The chosen `/etc` files

Only files where a change does what the user expects, and nothing that
can lock anybody out. The helper that writes them refuses every other
path (`locations.cpp`, `systemWritable()`):

- `/etc/default/grub` (boot menu: timeout, kernel options)
- `/etc/environment`, `/etc/default/locale`
- `/etc/systemd/logind.conf`, `journald.conf`, `system.conf`, `sleep.conf`,
  `timesyncd.conf`, `resolved.conf`, and the `*.conf` of their `.d` folders
- `/etc/sddm.conf` and `/etc/sddm.conf.d/*.conf` (sign-in screen)
- `/etc/NetworkManager/NetworkManager.conf` and `conf.d/*.conf`
- `/etc/snapper/configs/*` (restore points)
- `/etc/xdg/*` (the Defaults root)

Never shown: `/etc/fstab`, `/etc/sudoers*`, `/etc/shadow`, `/etc/passwd`,
PAM, ssh, the network connections (they hold Wi-Fi passwords), anything
not owned by root, anything that is a link.

## Types

Settings files only hold text. The editor shows each value with the type
it reads as, the way Windows shows REG_SZ or REG_DWORD, and checks a new
value against that type before writing it:

| Shown as | When | Accepted |
|---|---|---|
| Text | anything else | any text, on one line |
| Number | an integer | an integer |
| Yes/No | `true`/`false` (KConfig), `yes`/`no` (systemd, shell files) | the same two words, in the same spelling |
| List | a KConfig value with commas | any text |

A dconf value has a real type (GVariant) and keeps it: `b` is Yes/No,
`i u x t n q y` are numbers, `d` decimal, `s` text, `as` a list of text,
anything else is shown as its GVariant text. A new value must parse as the
type of the old one (`g_variant_parse` with that type), so a boolean
cannot become a string by accident.

New values made with the editor are text (files) or take the type the
user picks (dconf: Text, Number, Yes/No, List of text).

## Read only

- A KConfig entry or group marked `[$i]` (locked by the administrator):
  shown with a lock, never written.
- A whole file locked by `[$i]` on its first line.
- `HKEY_LOCAL_MACHINE\Information`.
- The system dconf databases (`/etc/dconf/db`): they are compiled from
  the files in `/etc/dconf/db/*.d`; the user's database is the one to edit.
- Values with `$` expansion (`[$e]`): shown as written, not expanded.

## Writing without breaking anything

1. **The file is edited, not rewritten.** Only the line of the value
   changes; comments, blank lines, the order of groups and the spelling of
   every other line stay as they were. A new value goes at the end of its
   group, a new group at the end of the file.
2. **Atomic.** The new file is written next to the old one and renamed
   over it (`QSaveFile`), so a crash or a full disk leaves the old file.
   Its mode is kept.
3. **A copy first.** Before each write the file as it was goes to
   `~/.local/state/raye-regedit/backups/<date>/` (user files) or
   `/var/backups/raye-regedit/<date>/` (system files), the 200 most recent
   kept.
4. **Undo.** Every change made in the window goes on an undo list
   (Ctrl+Z): the value goes back to what it was, or goes away if it was
   new. Undo works the same way as an edit, so it also gets a copy first.
5. **Re-read before writing.** The file is read again at the moment of the
   write, so a change made meanwhile by the app that owns the file is not
   thrown away.
6. **Values are escaped** the way each format wants: KConfig `\n \t \\`
   and leading spaces, shell files quoted when they need it.

### The admin part

The window never runs as root. For a `HKEY_LOCAL_MACHINE` change it runs
`pkexec /usr/libexec/raye-regedit-helper` (action
`org.raye.regedit.write`, `auth_admin_keep`: the password is asked once
for a few minutes). The helper:

- takes one change on its command line (`set` or `remove`, the path, the
  group, the key) and the value on its input, never a whole file;
- refuses a path that is not in the list above, a link, a file that is
  not a regular file owned by root, and a file over 512 KiB;
- makes the copy, does the edit with the same code the window uses, and
  writes atomically, keeping mode and owner.

## Export and import

`File > Export` writes the selected key and everything under it as text:

    Raye Registry Editor 1

    [HKEY_CURRENT_USER\Config\kdeglobals\General]
    "AccentColor"="123,60,255"
    "Name"=-

    [HKEY_CURRENT_USER\Desktop\org\gnome\desktop\interface]
    "color-scheme"=gvariant:'prefer-dark'

`=-` removes a value. Import shows how many values it is going to change
and where, and asks first. Each value goes through the same checks as an
edit by hand; a line that names a file the editor does not show is
skipped and counted.

## Favourites

`Favourites > Add` keeps the selected key under a name, in
`~/.config/raye/raye-regedit.conf`.

## Not written yet

- `HKEY_CLASSES_ROOT` (file associations, `mimeapps.list`).
- Watching files for changes made by other apps while the window is open
  (`View > Refresh`, F5, reads them again).
- Telling a running app to read its settings again: most KDE apps read
  them at start, some at once.
