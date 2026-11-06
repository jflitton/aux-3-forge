# pkg

`--pkg` · A/UX 3.1 · Any Mac

[pkg](../../pkg/README.md), a package manager for A/UX: it installs
software from package files or over the network, and upgrades and removes it cleanly. Its
README says how to use it. Over the network it needs `--daynaport`, or another network
card A/UX can use.

The patch puts pkg in `/usr/local/bin` and records it as an installed package, so `pkg
upgrade` upgrades pkg itself. It also adds `/usr/local/bin`, where packages put their
programs, to root's PATH; A/UX gives it to other users already.

## For developers

| File | What it is |
|---|---|
| `apply.sh DISK` | Applies the patch to a copy of a disk image |
| `update.sh` | Builds pkg in `pkg/` and copies it into `bin/` |
| `bin/pkg` | pkg, built from `pkg/` |
| `bin/record` | The start of pkg's record of itself: its version, and the tag or commit `bin/pkg` was built from |
