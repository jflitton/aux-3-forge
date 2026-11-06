#!/bin/sh
# Bring bin/netsetup-VERSION.pkg, the netsetup package this patch installs, up to date from
# ports/netsetup/, building it there with its own Makefile (needs m68k-elf-gcc and
# m68k-elf-binutils):
#   patches/daynaport/update-netsetup.sh
# ports/ and tools/ must have no uncommitted changes, so that the package is what's in git.
set -e
if [ $# -ne 0 ]; then
	echo "usage: $0" >&2
	exit 1
fi
cd "$(dirname "$0")"
top=$(cd ../.. && pwd)
if [ -n "$(git -C "$top" status --porcelain -- ports tools)" ]; then
	echo "ports/ or tools/ has uncommitted changes" >&2
	exit 1
fi
version=$(sed -n 's/^version //p' "$top/ports/netsetup/+PKG")
[ -n "$version" ] || { echo "no version in ports/netsetup/+PKG" >&2; exit 1; }
make -C "$top/ports" port-netsetup
mkdir -p bin
rm -f bin/netsetup-*.pkg
cp "$top/ports/build/packages/netsetup-$version.pkg" bin/
echo "bin/netsetup-$version.pkg, built from $(git -C "$top" rev-parse --short HEAD)"
