#!/bin/sh
# Build the lc520 kernel patch from source (needs m68k-elf-gcc and m68k-elf-binutils):
#   patches/lc520/build.sh KERNEL OUTDIR
# KERNEL is an A/UX 3.1 /unix. Links src/lc520.S (the Sonora module, modules/sonora) at
# orwellsize and the Cuda transport (modules/cuda/cuda.c) after cEgretDispatch, and writes
# OUTDIR/kernel.txt, the binpatch spec; the intermediate files (.elf, and .bin .bin2 for the
# raw .text .text2) go in build/lc520. CKPT=1 builds the checkpoint variant (see
# modules/common/hooks.S) as kernel-ckpt.txt.
set -e
abs() { case $1 in /*) echo "$1" ;; *) echo "$PWD/$1" ;; esac; }
kernel=$(abs "$1") outdir=$(abs "$2")
cd "$(dirname "$0")/../.."
p=patches/lc520 b=build/lc520
mkdir -p $b "$outdir"
python3 -I tools/ksyms-ld.py "$kernel" > $b/kernel.ld
python3 -I $p/kernel-patch.py "$kernel" ld > $b/hooks.ld
m68k-elf-gcc -m68030 -Os -ffreestanding -fno-builtin -nostdlib -fno-pic -fomit-frame-pointer \
	-fno-jump-tables -ffunction-sections -Wall -Werror -c modules/cuda/cuda.c -o $b/cuda.o
out=$b/kernel spec=$outdir/kernel.txt
defs=
if [ -n "$CKPT" ]; then out=$out-ckpt spec=$outdir/kernel-ckpt.txt defs=-DCKPT_BUILD; fi
m68k-elf-gcc -m68030 $defs -I modules -c $p/src/lc520.S -o $b/lc520.o
m68k-elf-ld -T $b/kernel.ld -T $b/hooks.ld -T $p/src/kernel.ld \
	-o $out.elf $b/lc520.o $b/cuda.o
m68k-elf-objcopy -O binary -j .text $out.elf $out.bin
m68k-elf-objcopy -O binary -j .text2 $out.elf $out.bin2
python3 -I $p/kernel-patch.py "$kernel" spec $out.elf $out.bin $out.bin2 > "$spec"
m68k-elf-size -A $out.elf | grep text
m68k-elf-nm -n $out.elf | grep -i ' t '
