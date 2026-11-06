# Prior art

What there was for building software for A/UX when this repository started
(researched 2026-10-30). None of it is used as code; [decisions.md](decisions.md), "Written
ourselves", says why, and how GCC's and binutils' old A/UX support serve as a description
of A/UX.

- **uyjulian/aux_crosscompile_toolchain**
  (https://github.com/uyjulian/aux_crosscompile_toolchain). No license; "not yet ready
  for users" (January 2025). Binutils 2.16.1 with an 11-line patch to read and write ELF;
  a script converts Apple's static `.a` files to ELF and deletes the `_s` libraries, so it
  links Apple's `libc.a` statically. Its release sysroot is Apple's headers and libraries.
  No GCC patches, no headers of its own. The same author's gist has the 1995 GCC 2.7.2.3
  A/UX patch (a native build, which needs Apple's assembler):
  https://gist.github.com/uyjulian/78f1234f82ad429ce43d4db00bd3b574
- **binutils.** `bfd/coff-aux.c` (GPLv3+). The configure target was obsolete in 2.17 and
  refused from 2.18, and the code was removed on 2018-04-16, so 2.30 is the last with it:
  https://sourceware.org/git/?p=binutils-gdb.git;a=blob;f=bfd/coff-aux.c;hb=refs/tags/binutils-2_30
  What it knew about shared libraries (`include/coff/aux-coff.h`, `coffcode.h`):
  - `STYP_LIB` 0x200: the `.lib` section, 64-byte padded pathnames of shared libraries.
  - `STYP_INIT` 0x400: shared-library initialisation code.
  - NOLOAD + BSS sections count as part of a shared library.
  - A static definition overrides a host library's absolute address ("mirrors Apple's
    'solution'").
  - Relocations sorted, as A/UX's `ld` wants; the executable magic numbers; an `m68kaux`
    ld emulation (`m68kaux.sc`, `m68kaux.sh`).

  nleymann.de warns that binutils 2.10.1's ld "can't cope with the Apple UX libraries".
- **GCC.** A/UX was obsolete in 3.1 and removed on 2002-05-24, so 3.2.3 is the last with
  it: https://github.com/gcc-mirror/gcc/tree/releases/gcc-3.2.3/gcc/config/m68k
  (`a-ux.h`, `auxas.h`, `auxgas.h`, `auxgld.h`, `aux-crt1.c`, `aux-exit.c`). From
  `a-ux.h`:
  - Results in d0, a0 (pointers, copied to d0 in the epilogue) or fp0.
  - `size_t` is `unsigned int`; structure sizes are a multiple of 2 bytes.
  - "We provide atexit(), A/UX does not have it".
  - Predefines `-Dunix -Dm68k -DAUX -DmacII`, and by default `_BSD_SOURCE`,
    `_SYSV_SOURCE`, `_AUX_SOURCE`.

  `aux-crt1.c` (GPLv2+ with a linking exception): `__splimit` from d0, then `initfpu()`,
  `__istart()` (the `.init` sections, for shared libraries), `__compatmode()`,
  `atexit(_cleanup)`, `exit(main(...))`.
- **The jagubox archive** (mirror:
  https://discmaster.textfiles.com/browse/11929/ftp.mayn.de-pub.zip/ftp.mayn.de-pub/apple/apple_unix).
  `GNU_stuff/gzip-124.bin.tar.Z` is binaries and manual pages only, no source or patches.
  Diffs for native builds of GCC 2.5.7 to 2.7.2 and gdb. Nothing for less or vim.
- **nleymann.de's A/UX Mini Archive** (https://nleymann.de/welcome/a-ux-mini-archive).
  Binaries only (gzip 1.2.4a, vim 5.8, binutils 2.10.1, GCC 2.8.1), no sources, no
  license.
- **chriskearney/aux-c89-summer** (https://github.com/chriskearney/aux-c89-summer, 2026,
  no license). X11R6, rxvt and others built inside A/UX under QEMU with GCC 2.7.2.3 and
  Apple's `mkshlib`; its notes and shims (`strdup`, `memmove`) hint at per-program
  problems.
- **corymalon/Apple_AUX_3.1.1_Setup** (MIT): install scripts only.
- **config.sub** (gnulib's copy of 2026-05-17) takes `m68k-apple-aux3.1`; `config.guess`
  gives `m68k-apple-aux$UNAME_RELEASE`.
