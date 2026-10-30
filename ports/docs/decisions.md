# Decisions

What we've settled about aux-3-ports, and why. Newest last. When a decision changes, edit
it here and say what replaced it, rather than leaving two answers.

## 2026-10-30: the first round

### A repository of its own

Ports, and the SDK that builds them, are kept out of aux-3-pkg, as Homebrew keeps its
formulae out of `brew` and FreeBSD its ports tree out of `pkg`. pkg stays buildable from
its own repository; ports change far more often and may come from other people; and ports
carry patches to GPL programs, which aux-3-pkg, MIT licensed, shouldn't. aux-3-pkg's
`tools/pkg-build.py` is used from a checkout beside this one.

The SDK and the ports start out together, because the SDK is only proven by building
ports. The SDK can move out if someone wants it alone.

### Cross-compiled, calling A/UX's shared libc

Ports are built on macOS with m68k-elf-gcc, as pkg is, so they get a current compiler and
need no emulator to build. They call A/UX's own shared C library, `/shlib/libc1_s`, at
the fixed addresses of its entry points: a program holds those addresses, not the
library's code, so no Apple code goes into it.

That library is only the core of libc: about 240 entry points, mostly stdio, the string
functions and the system calls. The rest, which A/UX's `/lib/libc_s.a` would copy into
each program (`malloc`, `exit`, `setjmp`, `strtol`, the time functions...), is ours, in
libaux, along with what a 1990 libc lacks (`snprintf`, `strerror`, `atexit`...).
Programs aren't changed to suit A/UX's libc: our headers describe its structures,
constants and types.

Rejected: growing pkg's `rt/` into a whole libc, a larger job when A/UX's shared library
has the core of one; and compiling on A/UX with 1990s gcc, which is slow, takes only old
C, and puts Apple's libc into every program.

### `libc1_s`, not `libc_s`

A/UX 3 has two shared C libraries. `/shlib/libc1_s` is the one `/lib/libc_s.a` links
against, and 565 of A/UX 3.1's programs use it. `/shlib/libc_s` is an older one, the
same file in every version, used only by 60 of its X11 programs; nothing on the disk
links against it any more.

### A/UX 3, all of it

`libc1_s` differs between A/UX 3.0, 3.0.1 and 3.1, but 234 of its entry points are at the
same addresses in all three, and those are all we use (`sdk/lib/libc1_s.ld`). The others,
`gettimeofday` and `settimeofday` (3.1 only) and `alphasort` (at a different address in
each), are left to libaux. So one build should run on all three. 3.1 is what we test:
QEMU's Quadra 800 can't boot A/UX 3.0's disk, and we have 3.0.1 only as an install CD.

### libaux is linked into each program

Not a shared library of its own. pkg has no dependencies yet, which a shared runtime
would need; an A/UX shared library sits at fixed addresses with a table of entry points
that can never be reordered, which is a lot to commit to while the library is young; and
it's small, and a program gets only the parts it uses. Worth revisiting if something big,
such as ncurses, ends up in many ports.

### A/UX's calling convention

Read from A/UX 3.1's libc. Arguments are on the stack, as gcc passes them. A pointer is
returned in both a0 and d0, and gcc reads d0. A double is returned in fp0, so ports are
built with `-m68881` (A/UX requires an FPU anyway), where pkg uses `-msoft-float`.

Where gcc differs: A/UX's code reads a returned pointer from a0. The shared library calls
the program's `malloc` and `realloc`, so libaux's return there too. Structures are
returned differently, so our headers leave out the few functions that return one.

### Written ourselves, with GCC's and binutils' old A/UX support as reference

Nothing in the community can be used as it is. The one modern attempt, uyjulian's
aux_crosscompile_toolchain, uses Apple's headers and links statically against Apple's
libraries converted to ELF, and has no license. The 1990s ports (the jagubox archive,
nleymann.de) survive as binaries, without their patches. So the headers, the linking
against `libc1_s`, the startup code and each program's patches are ours.

What A/UX expects is read from A/UX's own files, and from GCC 3.2.3's A/UX configuration
(`config/m68k/a-ux.h`, `aux-crt1.c`; GCC dropped A/UX in 3.3) and binutils 2.30's A/UX
COFF support (`bfd/coff-aux.c`, `include/coff/aux-coff.h`; dropped in 2.31). Both are GPL:
we read them as a description of A/UX and write our own code, so this repository stays
MIT.

### GPL programs come with their source

A GPL program's package is published with the source it was built from, beside the
package on the server: in `source/NAME-VERSION/`, the upstream tarball and a snapshot of
this repository (the files git would commit), which has the patches, the build scripts,
and libaux and the startup code, which are linked into the program. (2026-10-30: the
snapshot, not just the patches, as libaux is part of every program.)

## 2026-10-30: gzip, the first port

### Ports are configured as for any cross-compiler

`./configure --host=m68k-apple-aux3.1`, with `sdk/bin` first in PATH: it holds the
compiler and tools under the names configure looks for, so ports need no CC, AR or
RANLIB of their own (macOS's ar and ranlib can't index m68k archives). `sdk/config.site`
answers what configure would otherwise guess when it can't run a test program, about
libaux and A/UX: gnulib's guesses were wrong both ways, such as that open can't open
directories, which broke `gzip -r`. Only what's known goes in it.

### libaux has its own printf

A/UX's `_doprnt` predates C99: no long long, so `%lld` and gnulib's own printf, which
passes `%lld` down, printed 0. libaux's printf family does every conversion itself
except floating point, which it hands to `_doprnt` one at a time, as that does it well.
Also `snprintf`, `vsnprintf`, `asprintf`, `dprintf`. Not argument positions (`%1$d`),
which only translations use.

### Programs run in A/UX's POSIX mode

libaux's startup sets the `setcompat` flags `/lib/libposix.a` does: without
`COMPAT_BSDSIGNALS` the signal calls sigaction is built on fail. Less `COMPAT_EXEC`, so
that A/UX's own programs a port runs keep their usual System V behaviour.

### Local time from A/UX's zone files

libaux's localtime reads `/etc/zoneinfo/$TZ` as A/UX's does, so a port shows the same
times as A/UX's own commands, out-of-date daylight saving rules included.

### Ports are tested through pkg

A port's test installs its package with pkg in A/UX, checks the program, and removes the
package again: the package is what users get, so that's what's tested.

### gzip: the programs, not the scripts

gzip's zgrep, zdiff, zmore and the rest are shell scripts for a POSIX shell, and A/UX's
`/bin/sh` is older; they're left out until they're made to work. gunzip and zcat are
scripts too, so they're links to gzip instead, built with `-DGNU_STANDARD=0` so that gzip
acts on the name it's run by. `uncompress` is left out so as not to hide A/UX's own.
