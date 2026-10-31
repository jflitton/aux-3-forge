# A/UX 3 ports

Software for A/UX 3, Apple's Unix for 68k Macs, cross-compiled on macOS and packaged for
[pkg](https://github.com/jflitton/aux-3-pkg), A/UX's package manager.

**Status:** early. Two ports, built and tested on A/UX 3.1 (in QEMU): gzip 1.14, which is
on pkg's server (`pkg install gzip`) with its source beside it, and mksh R59c. less and
vim are next. And one program of our own, netsetup.

| Port | Version | What's in the package |
|---|---|---|
| gzip | 1.14 | `gzip`, `gunzip` and `zcat`, and their manual pages. Not gzip's shell scripts (`zgrep`, `zdiff`, `zmore`...), which need a newer shell than A/UX's |
| mksh | R59c | `mksh`, the MirBSD Korn shell, and its manual page: line editing with the arrow keys, tab completion, history and job control, which A/UX's `/bin/sh` lacks |
| netsetup | 1.0 | Ours: `netsetup` sets up a network interface by DHCP or by hand, `dhcpc` gets and keeps DHCP leases, and a Commando dialog for `netsetup`. Needs pkg 0.5 |

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

### netsetup

It works with any network interface A/UX has a driver for: `dp0`, the DaynaPORT that a
BlueSCSI or ZuluSCSI with Wi-Fi emulates (aux-3-patches' `--daynaport`, which puts
netsetup on the disk too), and Apple's Ethernet cards and built-in Ethernet (`ae0`,
`as0`, `ac0`, `ao0`), though only `dp0` has been tried. As root:

    netsetup                      how each interface is set up, and its address now
    netsetup dp0 dhcp             dp0's address by DHCP, now and at every boot
    netsetup dp0 192.168.1.50 netmask 255.255.255.0 gateway 192.168.1.1 dns 192.168.1.1
                                  an address set by hand, now and at every boot
    netsetup dp0 off              no address
    netsetup dns 192.168.1.1,8.8.8.8

In CommandShell, type `netsetup` and press Command-K for its dialog. A change to the
interface you're connected through drops a telnet session; it carries on regardless.

DHCP needs a server that answers clients asking for broadcast replies, as home routers
do. The lease is renewed in the background by `dhcpc`, which notes what happens in
`/usr/adm/dhcpc.log`. Mac programs that use MacTCP use A/UX's own network, so they get
the same settings.

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
