#!/bin/sh
# Every taskmanager source that needs only Qt must compile under the flags
# KDECompilerSettings sets. Syntax only: no linking, no KF6, no build tree.
#
#   sh taskmanager/tests/compile_probe.sh          (from the repository root)
#
# Why this exists. taskmanager/src/processmodel.cpp did not compile AT
# ALL -- on Qt 6.8 or 6.10 -- and six static test suites, 18 ctest checks
# and the launcher's own "11 tests passed" evidence all stayed green.
# `QString(QChar(row.stat.state))` where `state` is a plain char
# (proctable.h:40): KDECompilerSettings defines QT_NO_CAST_FROM_ASCII, and
# under it QChar(char) is `= delete`. Nothing that reads source as TEXT can
# see that; only a compiler can. Measured 2026-11-06: `error: use of
# deleted function QChar::QChar(char)`.
#
# main.cpp is left out, and that is named rather than silent: it is the
# only source here that includes a KF6 header, and KF6 is not installed on
# the machine this runs on. A probe that skipped files without saying so
# would be the vacuous green this tree keeps paying for -- so the count of
# what it compiled, and of what it set aside, is printed and asserted.
set -eu

here=$(cd "$(dirname "$0")/../.." && pwd)
src=$here/taskmanager/src
ecm=${RAYE_ECM_SETTINGS:-}

# The definitions are READ from KDECompilerSettings when it can be found,
# never written here: a hard-coded list freezes today's flags and goes
# green the day ECM adds one.
# The INSTALLED path, from the extra-cmake-modules package -- not a
# `find /`. The first version searched the whole filesystem and took
# whichever copy traversal reached first: on this machine that was a copy
# inside a 92 GB tree of agent scratch, and the day that tree is cleaned
# the probe would have fallen back to three hard-coded flags SILENTLY and
# still printed "ok". A guard that quietly covers less is the defect this
# file exists to catch. Measured 2026-11-06, caught in my own work.
: "${ecm:=/usr/share/ECM/kde-modules/KDECompilerSettings.cmake}"
[ -r "$ecm" ] || {
    echo "FAIL: $ecm is not readable, so the real flags cannot be read."
    echo "      This probe is worth nothing with guessed flags -- install"
    echo "      extra-cmake-modules, or point RAYE_ECM_SETTINGS at a copy."
    exit 1
}
defs=$(grep -oE "QT_NO_[A-Z_]+" "$ecm" | sort -u | sed 's/^/-D/' | tr '\n' ' ')
# And the list must not be empty: a file that stopped defining them
# would hand g++ no flags at all, and every source would compile.
case $defs in
    *QT_NO_CAST_FROM_ASCII*) ;;
    *) echo "FAIL: $ecm defines no QT_NO_CAST_FROM_ASCII; the flags read empty"; exit 1 ;;
esac
source_des_drapeaux=$ecm

inc=""
for m in Qt6Core Qt6Gui Qt6Qml Qt6DBus; do
    pkg-config --exists "$m" 2>/dev/null || { echo "FAIL: $m absent, rien n a pu etre compile"; exit 1; }
    inc="$inc $(pkg-config --cflags "$m")"
done

compiles=0; ecartes=0; fail=0

# An unexpanded glob must not reach the compiler. Without this, an empty
# src/ hands g++ the literal "*.cpp", g++ fails on a file that is not
# there, and the run goes red BEFORE the floors below can say why -- a
# proof that fires for the wrong reason is not a proof. Measured
# 2026-11-06 while proving the floors: the first attempt did exactly that.
for f in "$src"/*.cpp; do
    [ -e "$f" ] || { echo "FAIL: no source matched $src/*.cpp -- the folder moved, and this probe was about to compile nothing"; exit 1; }
    break
done

for f in "$src"/*.cpp; do
    if grep -qE "^#include <K" "$f"; then
        echo "note: $(basename "$f") ecarte -- il inclut un en-tete KF6, absent ici"
        ecartes=$((ecartes + 1))
        continue
    fi
    # shellcheck disable=SC2086
    if g++ -std=c++20 -fsyntax-only $defs $inc -I"$src" "$f" 2>/tmp/probe.err; then
        compiles=$((compiles + 1))
    else
        echo "FAIL: $(basename "$f") ne compile pas sous les drapeaux d ECM:"
        sed -n '1,6p' /tmp/probe.err | sed 's/^/    | /'
        fail=1
    fi
done

# The floors. Without them this script is green having compiled nothing
# -- a renamed folder, a missing pkg-config, a glob that matched no file.
[ "$compiles" -ge 4 ] || { echo "FAIL: only $compiles source(s) compiled; the probe read almost nothing"; fail=1; }
[ "$ecartes" -le 1 ] || { echo "FAIL: $ecartes source(s) set aside; the probe is covering less and less"; fail=1; }

[ "$fail" = 0 ] && echo "ok   $compiles taskmanager source(s) compile under ECM's flags ($ecartes set aside) -- flags from $source_des_drapeaux"
exit "$fail"
