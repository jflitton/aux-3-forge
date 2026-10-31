# To do

What isn't known or done yet. When an item is settled, take it out, and put what was
decided in [decisions.md](decisions.md) or what was learned in
[development.md](development.md).

## Testing

- **A/UX 3.0 and 3.0.1.** QEMU's Quadra 800 stops at the blinking-disk icon with
  `AUX_3_Use_in_Shoebill.zip` (3.0, `/etc/RELEASE_ID` March 9, 1992), as its Mac OS
  predates the Quadra 800. 3.0.1 is only to be had as an install CD. Shoebill, or a 3.0.1
  disk with a newer System, might do.
- **Installing from the server in A/UX.** Published packages have been installed from the
  server only by the macOS build of pkg, as A/UX has no network in QEMU.
- **gnulib's remaining guesses** (`configure.out`, "guessing"): ungetc on arbitrary bytes,
  lstat/stat/unlink/utime with trailing slashes, rmdir, memchr, utimes. Each costs a
  replacement wrapper, not a wrong answer. Running gnulib's test programs in A/UX and
  adding the answers to `sdk/config.site` would settle them.
- **Checking our COFF with binutils.** binutils 2.30 built with `m68k_coff_aux_vec`, just
  for its `objdump` ([prior-art.md](prior-art.md)).

## libaux

Still missing: strtod, strftime, asctime/ctime, execv/execvp, setvbuf,
tmpfile/tmpnam, getc/putc as macros (they're functions now), termios (tcgetattr... on
A/UX's termio ioctls), wide characters beyond the C locale. less and vim will need
several.

## A/UX

- **How A/UX returns a structure**: a1 and a static buffer, by GCC 3.2.3's defaults, where
  GCC 16's m68k-elf uses a0 and registers. libc has no `div` or `ldiv`; check what else
  returns one (`inet_makeaddr`?) before declaring it.
- **What `_ac_s` and `_ac_r` are** (data.o sets `_ac_r = &_ac_s`, shared data at
  0x47fc205c). Nothing seems to need them yet.
- **Whether A/UX's `man` finds pages in `/usr/local/man`.** It reads preformatted pages
  from `/usr/catman`, at least.

## Ports

- **less and vim**, next.
- **mksh in CommandShell.** Tested on a pseudo-terminal in QEMU, not yet in a
  CommandShell window: whether its VT102 emulation draws mksh's line editing right, and
  whether its windows have a suspend key (`stty -a`), which pseudo-terminals don't.
- **gzip's scripts** (zgrep, zdiff, zmore, znew, gzexe...) under A/UX's `/bin/ksh` (ksh88,
  which has `$(...)`): rewrite their `#!` and test them, then add them to the package.
  They also use `printf`, which A/UX lacks as a command.

## Packages

- **A `source` line in `+PKG`**, pointing at the package's `source/NAME-VERSION/` on the
  server, so `pkg info` says where its source is. gzip's would need a 1.14-2 to get one.
- **Reproducible packages.** A package's files keep the times they were built at, so
  building a port twice gives the same files in packages with different SHA-256s. Setting
  the staged files' times (to the source tarball's, say) before `pkg-build.py` would fix
  it.
