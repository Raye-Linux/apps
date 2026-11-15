#!/bin/sh
# Shows what a release of xdg-desktop-portal-kde changes in the files
# this backend carries (or in the files of any other manifest in the same
# format, such as the file dialog backend's).
#
#   sync-upstream.sh [--manifest FILE]... [--old TARBALL] NEW_VERSION|NEW_TARBALL
#       for each file of each manifest: unchanged, changed (with the
#       diff from the carried release to the new one) or removed; then
#       the files the new release adds or removes next to them.
#   sync-upstream.sh [--manifest FILE]... [--old TARBALL] --check
#       checks that the files marked as-is are still byte for byte the
#       carried release (edits belong in files marked edited).
#
# The tarball of the carried release is downloaded from the manifest's
# url (or taken from --old) and must match the manifest's sha256.
# Exit status: 0 nothing to do, 1 changes to look at, 2 error.
#
# Manifest lines (# starts a comment):
#   upstream <name>
#   version  <carried release>
#   url      <tarball url, @VERSION@ is replaced>
#   sha256   <checksum of the carried release's tarball>
#   file     <path in the tarball> <path next to the manifest> <sha256> as-is|edited

set -eu

here=$(cd "$(dirname "$0")" && pwd)
manifests=""
old_tarball=""
mode=diff
new=""

die() {
    echo "sync-upstream: $*" >&2
    exit 2
}

while [ $# -gt 0 ]; do
    case $1 in
        --manifest) [ $# -ge 2 ] || die "--manifest needs a file"; manifests="$manifests
$2"; shift 2 ;;
        --old) [ $# -ge 2 ] || die "--old needs a tarball"; old_tarball=$2; shift 2 ;;
        --check) mode=check; shift ;;
        -h|--help) sed -n '2,25p' "$0"; exit 0 ;;
        -*) die "unknown option $1" ;;
        *) [ -z "$new" ] || die "one release at a time"; new=$1; shift ;;
    esac
done
[ -n "$manifests" ] || manifests=$here/UPSTREAM
if [ "$mode" = diff ] && [ -z "$new" ]; then
    die "name the new release (a version or a tarball), or use --check"
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT INT TERM
cache=${XDG_CACHE_HOME:-$HOME/.cache}/raye-portal-sync

sha() {
    sha256sum "$1" | cut -d' ' -f1
}

# a path from a manifest: relative, no "..", nothing odd
safe_path() {
    case $1 in
        ''|/*|*..*|*'\'*|-*) return 1 ;;
    esac
    printf '%s' "$1" | grep -Eq '^[A-Za-z0-9_./@+-]+$'
}

fetch() { # url -> path of the downloaded file
    mkdir -p "$cache"
    out=$cache/$(basename "$1")
    case $(basename "$1") in
        *.tar.xz|*.tar.gz|*.tar.bz2) ;;
        *) die "not a tarball url: $1" ;;
    esac
    if [ ! -f "$out" ]; then
        curl --proto '=https' --tlsv1.2 -sSfL -o "$out.part" "$1" || die "cannot download $1"
        mv "$out.part" "$out"
    fi
    printf '%s' "$out"
}

unpack() { # tarball dir -> the tree's top folder
    mkdir -p "$2"
    tar -C "$2" --no-same-owner --no-same-permissions -xf "$1" || die "cannot unpack $1"
    top=$(find "$2" -mindepth 1 -maxdepth 1 -type d | head -n 1)
    [ -n "$top" ] || die "$1 holds no folder"
    printf '%s' "$top"
}

status=0
n=0
# errors inside the loop end it with status 2, and set -e passes that on
echo "$manifests" | while IFS= read -r manifest; do
    [ -n "$manifest" ] || continue
    [ -f "$manifest" ] || die "no manifest $manifest"
    n=$((n + 1))
    base=$(cd "$(dirname "$manifest")" && pwd)
    name=$(sed -n 's/^upstream[[:space:]]\{1,\}//p' "$manifest")
    version=$(sed -n 's/^version[[:space:]]\{1,\}//p' "$manifest")
    url=$(sed -n 's/^url[[:space:]]\{1,\}//p' "$manifest")
    want=$(sed -n 's/^sha256[[:space:]]\{1,\}//p' "$manifest")
    [ -n "$name" ] && [ -n "$version" ] && [ -n "$want" ] || die "$manifest: upstream, version and sha256 are needed"

    if [ -n "$old_tarball" ]; then
        old_tar=$old_tarball
    else
        [ -n "$url" ] || die "$manifest: no url and no --old"
        old_tar=$(fetch "$(echo "$url" | sed "s/@VERSION@/$version/g")")
    fi
    [ "$(sha "$old_tar")" = "$want" ] || die "$old_tar is not $name $version (sha256 differs)"
    old=$(unpack "$old_tar" "$work/old-$n")

    if [ "$mode" = diff ]; then
        if [ -f "$new" ]; then
            new_tar=$new
        else
            printf '%s' "$new" | grep -Eq '^[0-9][0-9.]*$' || die "not a version or a file: $new"
            [ -n "$url" ] || die "$manifest: no url to download $new"
            new_tar=$(fetch "$(echo "$url" | sed "s/@VERSION@/$new/g")")
        fi
        echo "== $name: $version -> $(basename "$new_tar") (sha256 $(sha "$new_tar"))"
        newtree=$(unpack "$new_tar" "$work/new-$n")
    else
        echo "== $name $version: as-is files"
    fi

    grep '^file[[:space:]]' "$manifest" | while read -r _ up local filesha kind; do
        safe_path "$up" && safe_path "$local" || die "$manifest: bad path in: $up $local"
        case $kind in as-is|edited) ;; *) die "$manifest: $up: as-is or edited, not '$kind'" ;; esac
        [ -f "$old/$up" ] || die "$up is not in $name $version"
        [ "$(sha "$old/$up")" = "$filesha" ] || die "$manifest: sha256 of $up does not match $version"
        [ -f "$base/$local" ] || die "$local is missing"
        if [ "$mode" = check ]; then
            if [ "$kind" = as-is ] && ! cmp -s "$old/$up" "$base/$local"; then
                echo "edited but marked as-is: $local"
                echo 1 > "$work/status"
            fi
            continue
        fi
        if [ ! -f "$newtree/$up" ]; then
            echo "removed upstream: $up ($kind $local)"
            echo 1 > "$work/status"
        elif cmp -s "$old/$up" "$newtree/$up"; then
            echo "unchanged: $up"
        else
            echo "changed: $up ($kind $local)"
            diff -u --label "a/$up" --label "b/$up" "$old/$up" "$newtree/$up" || true
            echo 1 > "$work/status"
        fi
    done

    if [ "$mode" = diff ]; then
        # files the release adds or removes in the folders carried from
        grep '^file[[:space:]]' "$manifest" | awk '{print $2}' | xargs -n1 dirname | sort -u > "$work/dirs-$n"
        while IFS= read -r d; do
            (cd "$old" && find "$d" -maxdepth 1 -type f 2>/dev/null | sort) > "$work/a"
            (cd "$newtree" && find "$d" -maxdepth 1 -type f 2>/dev/null | sort) > "$work/b"
            comm -13 "$work/a" "$work/b" | sed 's/^/new upstream file: /'
            comm -23 "$work/a" "$work/b" | sed 's/^/gone upstream: /'
        done < "$work/dirs-$n"
    fi
done
[ -f "$work/status" ] && status=$(cat "$work/status")
if [ "$status" = 0 ]; then
    echo "nothing to do"
fi
exit "$status"
