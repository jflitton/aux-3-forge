#!/bin/sh
# Bring bin/ up to date from pkg/, building pkg there with its own Makefile (needs
# m68k-elf-gcc and m68k-elf-binutils):
#   patches/pkg/update.sh
# writes bin/pkg, the A/UX executable, and bin/record, the start of the record pkg keeps
# of itself on the Mac: the +PKG of `make package`, with pkg's version and the pkg-v tag
# or commit it was built from. pkg/ and tools/ must have no uncommitted changes, so that
# bin/record says what's in bin/pkg.
set -e
if [ $# -ne 0 ]; then
	echo "usage: $0" >&2
	exit 1
fi
cd "$(dirname "$0")"
top=$(cd ../.. && pwd)
if [ -n "$(git -C "$top" status --porcelain -- pkg tools)" ]; then
	echo "pkg/ or tools/ has uncommitted changes" >&2
	exit 1
fi
make -C "$top/pkg" package
mkdir -p bin
cp "$top/pkg/build/aux/pkg" bin/pkg
cp "$top/pkg/build/package/+PKG" bin/record
echo "bin/pkg: pkg $(sed -n 's/^version //p' bin/record), built from $(sed -n 's/^built //p' bin/record)"
