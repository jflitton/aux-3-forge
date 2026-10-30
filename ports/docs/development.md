# Developing the SDK and ports

## Setup

    brew install m68k-elf-gcc m68k-elf-binutils qemu

The tools run on the system `python3` with only the standard library, as `python3 -I`.
Ports also need checkouts of aux-pkg (whose `tools/pkg-build.py` makes the packages) and,
for testing, aux-3-patches (whose tools read and write A/UX disks), beside this
repository. Tests boot A/UX in QEMU with aux-pkg's `tools/aux-run.py`, which needs a stock
A/UX 3.1 disk image (the Shoebill one, `AUX_3_1_1GB.dsk`) and a Quadra 800 ROM. Say where
those are in `local.mk`, which git ignores:

    AUX_DISK = /path/to/AUX_3_1_1GB.dsk
    AUX_ROM = /path/to/quadra_800.BIN

Set `AUX_PATCHES` and `AUX_PKG` there too to put the checkouts elsewhere.

## Building and testing

    make                  the SDK (build/sdk/) and its test programs (build/test/)
    make test-aux         the test programs, in A/UX 3.1 booted in QEMU (about 20 s)
    make port-NAME        ports/NAME's package, in build/packages/
    make test-port-NAME   that, then installed with pkg and tested in A/UX
    make clean            all of build/; the downloads in dist/ are kept

## Building other software with the SDK

With `sdk/bin` first in PATH, `./configure --host=m68k-apple-aux3.1` finds a compiler
and tools under that name: `m68k-apple-aux3.1-gcc` is `sdk/bin/aux-cc`, and `-ar`,
`-ranlib`, `-nm` and `-objdump` are m68k-elf's. Set `CONFIG_SITE=sdk/config.site` too, so
that configure knows what it can't find out by running a test program when
cross-compiling.

`aux-cc` compiles with `-m68020 -m68881` against `sdk/include` (then gcc's own headers,
for `stddef.h`, `stdarg.h` and the like). It links with `sdk/lib/aux.ld`,
`build/sdk/crt/crt0.o` first and `build/sdk/libaux.a` and `-lgcc` last, and writes the
result as COFF with `sdk/tools/elf2aout.py`. With `AUX_CC_KEEP_ELF=1` it keeps the ELF,
which has the symbols, as `OUT.elf`.

To find a crash: A/UX's `adb` reads a core file. `echo '$r' | adb PROGRAM core` shows
the registers and the faulting instruction, and `m68k-elf-nm -n OUT.elf` the function
it's in.

## Making a port

`ports/NAME/` holds:

| File | What |
|---|---|
| `+PKG` | The package's description (aux-pkg's docs/formats.md); its version is the program's, then `-N` for our build of it |
| `source` | The source tarball's URL and SHA-256, on one line |
| `*.patch` | Patches to the source, applied in name order with `patch -p1` |
| `build.sh` | Run by `/bin/sh -e` in a build directory, with `SRC` (the unpacked source), `STAGE` (where to install, laid out from /) and `TESTDIR` (files the test needs) set, `sdk/bin` first in PATH and `CONFIG_SITE` set |
| `aux-test.sh` | Run in A/UX, with pkg, the package and TESTDIR's files in `/usr/local/bin`; it installs the package with pkg, checks it, removes it, and exits non-zero if anything failed |

`tools/build-port.py` does the rest: the download (into `dist/`, checked against the
SHA-256 every time), unpacking and patching (into `build/ports/NAME/`), the package, and a
copy of the tarball and patches in `build/packages/source/`.

## Publishing a port

To pkg's own server (aux-pkg's docs/publishing.md has the general steps), with the
signing key in `.env` (ignored by git) and the AWS CLI signed in to the server's bucket:

    make port-NAME
    mkdir -p build/site/packages && cp build/packages/NAME-VERSION.pkg build/site/packages/
    python3 -I ../aux-pkg/tools/pkg-repo.py index build/site .env
    ../aux-pkg/tools/publish.sh build/site s3://BUCKET http://auxpkg.zerobytesfree.org/

`build/site/packages/` must hold every package already on the server, as the list
names them all. Then the source, into `source/NAME-VERSION/` beside it
(docs/decisions.md, "GPL programs come with their source"): the upstream tarball, and
`aux-3-ports.tar.gz`, the files `git ls-files --cached --others --exclude-standard` lists,
under `aux-3-ports/`. Like packages, these never change once they're up; upload them with
`--cache-control "public, max-age=31536000, immutable"`.

## Test scripts

Test scripts run under A/UX's `/bin/sh`, a System V Bourne shell: no `!`, no `$(...)`,
`test` has no `-h`, `mkdir` no `-p`, and there's no `printf` command.

## Layout

| Path | What |
|---|---|
| `sdk/include/` | Headers describing A/UX's libc, for what the shared library and libaux have |
| `sdk/libaux/` | libaux: what A/UX's libc has outside the shared library, and what it lacks |
| `sdk/crt/crt0.S` | Program entry |
| `sdk/lib/aux.ld` | The executable's layout; `libc1_s.ld`, the shared library's addresses |
| `sdk/bin/` | `aux-cc`, and the tools under the names `./configure --host=m68k-apple-aux3.1` looks for |
| `sdk/config.site` | Answers for configure about libaux and A/UX |
| `sdk/tools/` | `elf2aout.py` (ELF to A/UX COFF, from aux-pkg's), `shlib-table.py` (writes `libc1_s.ld`) |
| `ports/` | One directory per port |
| `tools/build-port.py` | Builds a port into a package, and tests it |
| `test/` | The SDK's test programs, and `aux-test.sh`, which runs them in A/UX |
| `dist/` | Downloaded source tarballs (ignored by git) |

## How a program uses A/UX's shared C library

Found from A/UX 3.1's own programs, `/lib/libc_s.a`, `/lib/crt1.o` and
`/lib/libposix.a`; the SDK follows them.

- **The executable.** As a static one (aux-pkg's development.md has the layout), plus a
  section of type `STYP_LIB` (0x200) named `.lib`, after `.data` in the file, holding
  `/shlib/libc1_s` padded with zeros to 64 bytes. The kernel maps the library's text at
  0x47f00000 and a private copy of its data at 0x47fc0000. A/UX's ld also writes
  headers for the library's sections, at those addresses and marked not to be loaded;
  the kernel doesn't need them. `.bss` must follow `.data`: if a program aligns `.bss`
  further, `elf2aout.py` pads `.data` to meet it.
- **Entry points.** Functions are 6-byte `jmp` entries in a table from 0x47f02000;
  data, such as `errno`, are at fixed addresses from 0x47fc2000. `/lib/libc_s.a` gives
  each as an absolute symbol, and `sdk/tools/shlib-table.py` writes those that are the
  same in each version as `sdk/lib/libc1_s.ld`.
- **The program's side.** The library uses some of the program's own data and functions,
  through pointers at 0x47fc2000 to 0x47fc2028 that the program fills in as it starts:
  `_ctype`, `malloc`, `free`, `realloc`, `_smbuf`, `_iob`, `&_lastbuf`, `_bufendtab`,
  `&environ`, `end` and `_cleanup`. A/UX's ld does it with `.init` sections from
  `/lib/libc_s.a`; libaux's `__aux_init` does it, called from `crt0`.
- **stdio's data** is the program's: `FILE _iob[128]` (14 bytes each: `_cnt`, `_ptr`,
  `_base`, `_flag`, `_file`), with stdin reading, stdout writing and stderr writing
  unbuffered into `_smbuf[2]`; `_smbuf[129][8]`; `_lastbuf = &_iob[128]`; and
  `_bufendtab[129]`, where each stream's buffer ends.
- **Compatibility mode.** A/UX gives each process a set of `setcompat` flags that choose
  between System V and BSD behaviour. The BSD signal calls (`sigvec`, `sigblock`...) fail
  with EINVAL unless `COMPAT_BSDSIGNALS` is set. `/lib/libposix.a` sets its POSIX set at
  startup (`0xc90d`); libaux sets the same less `COMPAT_EXEC`.
- **Startup**, in A/UX's `crt1.o`: save the stack limit from d0, set `environ`, call
  `initfpu`, the `.init` sections and `__compatmode` (the first is empty; the last is
  empty in libc, `setposix` in libposix and `setbsd` in libbsd), then `main`, then `exit`,
  which calls `_cleanup` and `_exit`. `crt0` keeps argc, argv and envp in registers and
  pushes them again for each call, as gcc's code may change the arguments it was passed.
