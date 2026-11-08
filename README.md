# aux-3-forge

More Macs and more software for A/UX 3, Apple's Unix for 68k Macs. No Apple software is
included.

- **Make a disk.** One command, `make-disk.sh`, makes a patched copy of an A/UX 3.1 disk
  image with the patches you pick: add support for previously unsupported macs,
  networking over ZuluSCSI/BlueSCSI Wi-Fi, and more. Your original image remains intact.
- **Add software.** [pkg](pkg/README.md) is a package manager that runs in A/UX: it
  installs, upgrades and removes software over the network, checking every download
  against a signed list.
- **Software to add.** [The ports](ports/README.md) are programs built for A/UX and
  packaged for pkg (so far gzip, the mksh shell and netsetup), with the kit that
  cross-compiles them on a modern Mac.

## Making a disk

Pick the patches you want:

| Patch | Option | What it does | A/UX | Macs |
|---|---|---|---|---|
| [Quadra 605](patches/q605/README.md) | `--enabler` with System Enabler 065 | Runs A/UX on the Quadra 605, and in theory the LC/Performa 475 | 3.1 | Quadra 605, LC 475 and Performa 475/476 with a full 68040 |
| [LC III](patches/lc3/README.md) | `--enabler` with System Enabler 003 | Runs A/UX on the LC III and LC III+ | 3.1 | LC III, LC III+, Performa 450/460/466/467 with an FPU; tested on an LC III at 33 MHz |
| [LC 520, LC 550, Color Classic II](patches/lc520/README.md) | `--enabler` with System Enabler 403 | **Untested:** should run A/UX on the LC 520, LC 550 and Color Classic II, but no one has tried it yet | 3.1 | LC 520, LC 550, Color Classic II, Performa 275/520/550/560 with an FPU; all untested |
| [DaynaPORT](patches/daynaport/README.md) | `--daynaport` | Networking over a BlueSCSI's or ZuluSCSI's Wi-Fi | 3.1 | Tested on the Quadra 605 and LC III |
| [Date](patches/date/README.md) | `--date` | `date` works with years after 1999 | 3.1 | Any |
| [pkg](patches/pkg/README.md) | `--pkg` | A package manager: installs software from package files or over the network | 3.1 | Any |

For a Mac that A/UX 3.1 doesn't know, give `--enabler` the System Enabler that Mac needs
for System 7.1, as you downloaded it: the enabler picks the patch for your Mac, and goes on
the disk too. A disk is for one of these Macs.

### Requirements

- macOS or Linux, with Python 3.9 or later.
- hfsutils and unar, for `--enabler` (`brew install hfsutils unar` on macOS).
- An A/UX 3.1 disk image. Tested with `AUX_3_1_1GB_Use_In_Shoebill.zip`, widely archived
  online; pass the zip or the `.dsk` inside it. (`AUX_3_Use_in_Shoebill.zip` is A/UX 3.0,
  which won't work.)
- Whatever your patches need (see their pages).

### Usage

    ./make-disk.sh AUX-IMAGE NEW-DISK PATCHES... [--ip ADDRESS ...]

A Quadra 605 disk with every patch, and an LC III disk:

    ./make-disk.sh AUX_3_1_1GB_Use_In_Shoebill.zip HD0_AUX.hda \
        --enabler System_Enabler_065_12-sea.bin --daynaport --date --pkg --ip 192.168.1.50
    ./make-disk.sh AUX_3_1_1GB_Use_In_Shoebill.zip HD0_AUX.hda \
        --enabler System_Enabler_003_1.1.sea.bin --date

`--help` lists every option. If a step fails, the partial disk is removed and the log is
left in `NEW-DISK.log`; each patch's page covers its own problems.

### Running A/UX

The disk boots Mac OS 7.1, then A/UX Startup launches A/UX, logged in as `root` with no
password. CommandShell, in the Apple menu, is the shell. Shut down with **Special → Shut
Down**: a power cut can damage the A/UX file system.

## Adding software

A disk made with `--pkg` has pkg. Over the network it needs `--daynaport` too, or another
network card A/UX can use. In CommandShell, as root:

    pkg update                    get the list of packages
    pkg list -a                   see what's there
    pkg install mksh              install one
    pkg upgrade                   bring everything up to date, pkg included

[pkg's page](pkg/README.md) has the rest, including how to make packages of your own, and
[the ports' page](ports/README.md) says what's in each package.

## For developers

The patches are at the top (`make-disk.sh`, `patches/`, and `modules/` for code the Mac
patches share), pkg is in `pkg/`, and the ports and the kit that builds them are in
`ports/`. [docs/development.md](docs/development.md) starts with how the parts fit
together.

## License

MIT; see [LICENSE](LICENSE).

## Disclaimer

This project is provided "as is", without warranty of any kind, express or implied,
including the warranties of merchantability and fitness for a particular purpose. A patched
operating system can lose data or leave a Mac unable to start; you use it at your own risk,
so keep backups. The authors are not liable for any damage or
loss arising from its use.

This project isn't affiliated with or endorsed by Apple. A/UX, Macintosh, Performa and
Quadra are trademarks of Apple Inc.; other names are trademarks of their owners.
