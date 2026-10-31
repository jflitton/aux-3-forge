# A/UX 3 ports

Software for A/UX 3, Apple's Unix for 68k Macs, cross-compiled on macOS and packaged for
[pkg](https://github.com/jflitton/aux-3-pkg), A/UX's package manager.

**Status:** early. Two ports, built and tested on A/UX 3.1 (in QEMU): gzip 1.14, which is
on pkg's server (`pkg install gzip`) with its source beside it, and mksh R59c. less and
vim are next.

| Port | Version | What's in the package |
|---|---|---|
| gzip | 1.14 | `gzip`, `gunzip` and `zcat`, and their manual pages. Not gzip's shell scripts (`zgrep`, `zdiff`, `zmore`...), which need a newer shell than A/UX's |
| mksh | R59c | `mksh`, the MirBSD Korn shell, and its manual page: line editing with the arrow keys, tab completion, history and job control, which A/UX's `/bin/sh` lacks |

### mksh

Type `mksh` in CommandShell to start it. To keep its history between sessions and have
control-Z stop a job, put these lines in `~/.mkshrc`, which it reads as it starts:

    HISTFILE=$HOME/.mksh_history
    stty susp '^Z'

To make it your login shell, `chsh yourname /usr/local/bin/mksh`, then log out and back
in: CommandShell starts the shell you had when you logged in. (Installing mksh adds it to
`/etc/shells`, which `chsh` needs; it takes pkg 0.3 or later.) Leave root's shell as
`/bin/sh`. pkg won't remove mksh while it's someone's login shell; `chsh yourname` puts
yours back to `/bin/sh`.

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
| `docs/` | [development.md](docs/development.md), [decisions.md](docs/decisions.md), [todo.md](docs/todo.md), [prior-art.md](docs/prior-art.md) |

## License

MIT for this repository's own files; see [LICENSE](LICENSE). A port's patches are under
the license of the program they patch, and a package of a GPL program is published with
its source.
