#!/bin/sh
# SPDX-FileCopyrightText: 2026 Raye Linux
# SPDX-License-Identifier: LGPL-2.1-or-later
# sync-upstream.sh on two small fake releases, offline.
# usage: test_sync_upstream.sh <sync-upstream.sh> <folder for the work>
set -eu
script=$1
mkdir -p "$2"
# a new folder each run, in the build tree
work=$(mktemp -d "$2/run.XXXXXX")
cd "$work"

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

# two releases of a fake upstream
mkdir -p v1/fake-1.0/src v2/fake-1.1/src
printf 'one\n' > v1/fake-1.0/src/a.cpp
printf 'two\n' > v1/fake-1.0/src/b.cpp
printf 'gone\n' > v1/fake-1.0/src/c.cpp
printf 'one\nmore\n' > v2/fake-1.1/src/a.cpp
printf 'two\n' > v2/fake-1.1/src/b.cpp
printf 'new\n' > v2/fake-1.1/src/d.cpp
tar -C v1 -cJf fake-1.0.tar.xz fake-1.0
tar -C v2 -cJf fake-1.1.tar.xz fake-1.1

# what we carry: a and c as-is, b edited
mkdir -p carry/src
cp v1/fake-1.0/src/a.cpp v1/fake-1.0/src/c.cpp carry/src/
printf 'two, Raye\n' > carry/src/b.cpp
sha() { sha256sum "$1" | cut -d' ' -f1; }
cat > carry/UPSTREAM <<EOT
upstream fake
version 1.0
sha256 $(sha fake-1.0.tar.xz)
file src/a.cpp src/a.cpp $(sha v1/fake-1.0/src/a.cpp) as-is
file src/b.cpp src/b.cpp $(sha v1/fake-1.0/src/b.cpp) edited
file src/c.cpp src/c.cpp $(sha v1/fake-1.0/src/c.cpp) as-is
EOT
run() {
    set +e
    HOME=$work XDG_CACHE_HOME=$work/cache sh "$script" --manifest carry/UPSTREAM --old fake-1.0.tar.xz "$@" > out.txt 2>&1
    rc=$?
    set -e
}

run --check
[ $rc = 0 ] || { cat out.txt; fail "check of untouched files"; }

printf 'one, edited by mistake\n' > carry/src/a.cpp
run --check
[ $rc = 1 ] || fail "check must see the edited as-is file (rc $rc)"
grep -q 'edited but marked as-is: src/a.cpp' out.txt || fail "check names the file"
cp v1/fake-1.0/src/a.cpp carry/src/a.cpp

run fake-1.1.tar.xz
[ $rc = 1 ] || { cat out.txt; fail "a new release with changes gives 1 (rc $rc)"; }
grep -q '^changed: src/a.cpp (as-is src/a.cpp)' out.txt || fail "a.cpp changed"
grep -q '^+more' out.txt || fail "the diff is shown"
grep -q '^unchanged: src/b.cpp' out.txt || fail "b.cpp unchanged"
grep -q '^removed upstream: src/c.cpp' out.txt || fail "c.cpp removed"
grep -q '^new upstream file: src/d.cpp' out.txt || fail "d.cpp is new"

run fake-1.0.tar.xz
[ $rc = 0 ] || fail "the same release: nothing to do (rc $rc)"
grep -q 'nothing to do' out.txt || fail "says nothing to do"

# a wrong old tarball is refused
run --old fake-1.1.tar.xz --check
[ $rc = 2 ] || fail "wrong tarball must stop (rc $rc)"

# paths that leave the tree are refused
sed -i 's#^file src/a.cpp src/a.cpp#file src/a.cpp ../../etc/passwd#' carry/UPSTREAM
run --check
[ $rc = 2 ] || fail "a path with .. must stop (rc $rc)"
grep -q 'bad path' out.txt || fail "says bad path"

# a version that is not one is refused before any download
sed -i 's#^file src/a.cpp ../../etc/passwd#file src/a.cpp src/a.cpp#' carry/UPSTREAM
run '1.0;ls'
[ $rc = 2 ] || fail "a bad version must stop (rc $rc)"

echo "sync-upstream: all checks passed"
