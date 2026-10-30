# A/UX 3 ports

Software for A/UX 3, Apple's Unix for 68k Macs, cross-compiled on macOS and packaged for
[pkg](https://github.com/jflitton/aux-pkg), A/UX's package manager.

**Status:** early. One port, gzip 1.14, built and tested on A/UX 3.1 (in QEMU) and on
pkg's server (`pkg install gzip`), with its source beside it; less and vim are next.

| Port | Version | What's in the package |
|---|---|---|
| gzip | 1.14 | `gzip`, `gunzip` and `zcat`, and their manual pages. Not gzip's shell scripts (`zgrep`, `zdiff`, `zmore`...), which need a newer shell than A/UX's |

## How it works

Programs are compiled with m68k-elf-gcc and call A/UX's own shared C library,
`/shlib/libc1_s`, whose entry points are at the same addresses in A/UX 3.0, 3.0.1 and
3.1. A program holds only those addresses, not the library's code, so no Apple code goes
into it. That library is the core of libc (stdio, strings, system calls); libaux, ours,
has the rest, such as `malloc`, the time functions, a printf with C99's conversions and
the POSIX signal calls, and is linked into each program. Programs aren't changed to suit
A/UX: our headers describe it, and `./configure --host=m68k-apple-aux3.1` works.

Building a port: `make port-gzip` makes `build/packages/gzip-1.14-1.pkg`, and `make
test-port-gzip` then installs it with pkg in A/UX and tests it. See
[docs/development.md](docs/development.md).

## Layout

| Path | What |
|---|---|
| `sdk/` | The headers, libaux, the startup code, the linker scripts with the shared library's addresses, `aux-cc` and the tools that make an A/UX program |
| `ports/NAME/` | One per program: where its source comes from, with its SHA-256; any patches; how it's built and tested; its `+PKG` |
| `tools/` | `build-port.py`, which builds a port into a package and tests it |
| `test/` | Programs that test the SDK in A/UX |
| `docs/` | [development.md](docs/development.md), [decisions.md](docs/decisions.md) |

## License

MIT for this repository's own files; see [LICENSE](LICENSE). A port's patches are under
the license of the program they patch, and a package of a GPL program is published with
its source.
