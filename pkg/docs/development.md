# Developing pkg

pkg is the `pkg/` folder of aux-3; [the repository's guide](../../docs/development.md) says
how it fits with the patches and the ports. Paths here are within `pkg/`, where `make`
runs, except `tools/`, which is at the top of the repository.

## Layout

| Path | What |
|---|---|
| `src/` | pkg: `main.c` (commands), `install.c` (install, remove, verify), `repo.c` (servers, the signed lists, downloads), `db.c` (records of what's installed), `http.c`, `tar.c`, `gunzip.c`, `ed25519.c`, `sha.c`, `util.c`; `defaults.h`, the default server and keys |
| `rt/` | The runtime pkg runs on under A/UX instead of Apple's libc: `crt0.S`, `sys.S` (syscalls), `libc.c`, `aux.h`, and `aux.ld`, the executable's layout |
| `tools/` (at the top) | `pkg-build.py`, `pkg-repo.py` and `ed25519.py` for package makers and publishers; `publish.sh` (puts packages on a server kept in S3) and `pkg-publish.py` (adds packages to one, starting from what it has); `pkg-release.sh` (`make release`); `elf2aout.py` (ELF to A/UX COFF) and `aux-run.py` (runs a script in A/UX in QEMU) for the build and tests |
| `test/` | `unit.c` (with vectors from `gen-unit.py`), `host-test.py` (end to end on this machine), `rt-test.c` (the runtime, in A/UX), `aux-test.sh` (all of it in A/UX, with `httpd.c` and `make-repo.py`) |
| `docs/` | This guide, [decisions.md](decisions.md), [formats.md](formats.md), [publishing.md](publishing.md) |
| `build/` | Everything built (ignored by git) |

## Setup

    brew install m68k-elf-gcc m68k-elf-binutils qemu

The tools run on the system `python3` with only the standard library, as `python3 -I`.
For `make test-aux`: a stock A/UX 3.1 disk image (the Shoebill one, `AUX_3_1_1GB.dsk`)
and a Quadra 800 ROM for QEMU. Say where they are in `local.mk` at the top of the
repository, which git ignores and the ports' Makefile reads too. Give whole paths, as
it's read from `pkg/` and `ports/`:

    AUX_DISK = /path/to/AUX_3_1_1GB.dsk
    AUX_ROM = /path/to/quadra_800.BIN

## Building and testing

    make              build/aux/pkg for A/UX; build/host/pkg for this machine
    make test         unit tests, then pkg end to end against a local server (seconds)
    make test-aux     the runtime's tests, the unit tests and pkg end to end, in A/UX
                      booted in QEMU (about 25 s)
    make package      build/packages/pkg-VERSION.pkg, pkg as a package for its server

pkg is built twice from the same source. For A/UX: m68k-elf-gcc (68020, no FPU) against
`rt/`, linked by `rt/aux.ld` and written as COFF by `tools/elf2aout.py`; no Apple code is
in it, and the same source always makes the same file. For this machine: its own
compiler and libc, with UndefinedBehaviorSanitizer (not AddressSanitizer: on macOS 26 a
program built with it hangs before `main`). `src/sys.h` picks between the two.

`make test-aux` boots a clone of the disk (`tools/aux-run.py`): the programs and test files
go into `/usr/local/bin`, an inittab `once` line runs `test/aux-test.sh` after `/etc/rc`,
and its output comes back through a log on the disk, which the harness reads while QEMU
runs. QEMU's Quadra 800 has an Ethernet chip (SONIC) A/UX 3.1 has no driver for, so the
network tests use `test/httpd`, an HTTP server (and DNS server answering everything with
127.0.0.1) on A/UX's loopback interface. pkg uses the same socket calls there as on a real
network. If a run doesn't finish, `build/run/screen.png` shows the screen.

## Releasing pkg

A new pkg gets a new `PKG_VERSION` in `src/pkg.h`. Commit it on main, then:

    make release-dry-run    everything short of the server and GitHub
    make release

`make release` (`tools/pkg-release.sh`) first checks what it can: `pkg/` and `tools/`
committed, the version new on GitHub and on the server, the key that signs the server's
list (`.env` at the top of the repository, or `PKG_KEYFILE`), the server's bucket in
`PKG_S3` (`s3://BUCKET`, from the environment: put `PKG_S3=s3://BUCKET` in `.env` and
direnv's `.envrc` exports it; the repository says nothing about how the server is
hosted), and the AWS CLI and `gh` signed in. Then it:

1. runs `make test` and `make test-aux`;
2. tags the commit `pkg-vVERSION` and makes `build/packages/pkg-VERSION.pkg` (`make
   package`), whose record names the tag;
3. adds it to the default server with `tools/pkg-publish.py`, which fetches what the
   server has, checks it against the signed list, signs the new list and uploads;
4. pushes main and the tag;
5. makes the GitHub release: `pkg` and `pkg-VERSION.pkg` as the server has them, which
   "Getting pkg" in the README links to, and the commits since the last release.

If the package doesn't get onto the server, the tag is taken off again and nothing has
been pushed. If the push fails after that, push by hand and `make release-github` does
step 5.

A disk made with `--pkg` upgrades its pkg itself, so `patches/pkg/update.sh` is needed
only when a patch needs the new pkg. (Until 2026-11-06, when pkg was its own repository,
the tags were `vVERSION`; they were renamed.)

## How A/UX runs pkg

Found from A/UX 3.1's own executables, libc and headers; `rt/` follows them.

- **Executables.** A static program is COFF (magic 0x150) with an a.out header of magic
  0413: 168 bytes of headers, `.text` from file offset and address 0xa8, `.data` straight
  after it in the file and at 0x400000 plus its file offset modulo 8KB, then `.bss`. The
  kernel starts at 0xa8 with argc, argv and envp on the stack.
- **Syscalls.** `trap #0` for the System V calls, arguments on the stack as the caller
  pushed them; `trap #15` for the BSD-style ones (sockets, `stat`, `mkdir`, `rename`...),
  arguments in a0, d1, a1, d2, a2, d3. The call number in d0 either way; the result in
  d0, or carry set and the error in d0. `rt/sys.S` has the numbers, read from libc's
  stubs.
- **Structures.** As in A/UX's headers, with longs 2-byte aligned (gcc's m68k default
  too). `struct stat` holds both System V (16-bit) and POSIX (32-bit) uid, gid and inode
  fields; the kernel fills both.
- **Quirks.** The System V `time` call fails from our code, so `time()` uses
  `gettimeofday`. `gettimeofday` is 3.0.1's call (0x88), which 3.1 keeps beside its own.
  A/UX's `/etc/hosts` writes addresses in hex (`0x7F.0x00.0x00.0x01`). The kernel runs
  `#!` scripts. init starts an inittab command with no standard input, so a script it
  runs directly is read from fd 0, and `exec <...` in it cuts the script off.

## A/UX 3.0.1

Its libc makes every syscall pkg uses the same way as 3.1's, except `gettimeofday`
(above), and the headers pkg follows are the same apart from a macro. Not yet run there.
