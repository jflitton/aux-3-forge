# LC 520, LC 550 and Color Classic II

`--enabler System_Enabler_403_1.0.2.sea.bin` · A/UX 3.1 · LC 520, LC 550, Color Classic II
(and the Performa 275, 520, 550 and 560) with an FPU; **untested**

Runs A/UX 3.1 on the Macs built on Apple's Sonora chip with the Cuda system controller:
the LC 520 and Performa 520, the LC 550 and Performa 550 and 560, and the Color Classic II
and Performa 275. A/UX knows their hardware (a 68030, a 5380 SCSI chip, built-in video with
its own video memory) from the Mac IIvx, but not the Macs themselves, and stops with
"unsupported motherboard type".

**Nobody has run this yet,** on a real Mac or in an emulator. It puts together two pieces
that work: the LC III patch's support for the Sonora chip, and the Quadra 605 patch's
support for the Cuda controller (keyboard, mouse, clock, power). If you try it, please
open an issue saying how far it got.

It adds System Enabler 403 to the disk's Mac OS, sends these Macs down the kernel's IIvx
path, talks to their Cuda, sizes their memory safely, sets up their sound hardware, and
lets System 7.1 accept them. Other Macs don't need it. A disk is for one group of Macs: a
disk with this patch is only for these, since it reuses space in the kernel that the
Quadras with Orwell and djMEMC memory controllers need, and A/UX's Egret code, which the
LC III and other Egret Macs need.

## You need

- **An FPU.** These Macs have a socket for a 68882 floating-point unit, often empty, and
  A/UX requires one.
- **At least 8 MB of memory,** and 16 MB or more to keep A/UX's Find File daemon happy.
  These Macs have 4 MB on the logic board and one 72-pin SIMM slot, up to 36 MB in all.
- **System Enabler 403**, version 1.0.2 is the one looked at, usually
  `System_Enabler_403_1.0.2.sea.bin` (widely archived online), as downloaded. Give it to
  `make-disk.sh` with `--enabler`, which picks this patch for it.
- hfsutils and unar (on a Mac, `brew install hfsutils unar`).

## Compatibility

| | Tested | Not tested yet | Won't work |
|---|---|---|---|
| **Mac** | | LC 520, LC 550, Color Classic II, Performa 275, 520, 550, 560 | Macintosh TV (Sonora and Cuda, but another ROM and enabler) |
| **Processor** | | 68030 with a 68882 | No FPU |
| **A/UX disk** | | `AUX_3_1_1GB_Use_In_Shoebill.zip` | |
| **With** | | `--date`, `--daynaport` | The Quadra 605 and LC III patches (System Enablers 065, 003) |

The Color Classic II's built-in screen is 512×384, small for A/UX but workable: A/UX also
runs on the SE/30's 512×342.

## Troubleshooting

**A/UX Startup says "No FPU":** fit a 68882 in the FPU socket.

**"Command is respawning too rapidly" for `catsearchd`:** A/UX found less than 16 MB of
memory, the minimum its Find File daemon asks for.

**The screen goes dark after "Launching…" and stays dark:** make a checkpoint disk, which
restarts the Mac (with the startup chime) at a point of your choosing, to find where it
stops. Build it with `CKPT=1 patches/lc520/apply.sh DISK ENABLER` on a copy of an A/UX
disk (this needs the m68k-elf cross compiler; see
[development.md](../../docs/development.md)). Then hold a digit key from "Launching…"
until the screen goes dark:

| Key | Restarts once the kernel has | Key | Restarts once the kernel has |
|---|---|---|---|
| 1 | started | 6 | set up the Cuda (clock, PRAM) |
| 2 | identified the Mac | 7 | set up the video |
| 3 | sized the memory | 8 | set up the keyboard and mouse |
| 4 | turned on the memory management unit | 9 | started its drivers (SCSI) |
| 5 | set up the VIAs | 0 | mounted the A/UX disk |

The highest key that still restarts the Mac tells where it stops. Restarting after key 0
leaves the disk to be checked at the next start ("Checking…"); that is expected.

## For developers

| File | What it is |
|---|---|
| `apply.sh DISK ENABLER` | Applies the patch to a copy of a disk image |
| `build.sh KERNEL OUTDIR` | Builds `bin/` from `src/` and the modules (needs the m68k-elf cross compiler) |
| `kernel-patch.py` | Puts the Sonora module's and the Cuda transport's patch sites together; writes the kernel patch |
| `src/lc520.S` | The kernel hooks: which Macs these are (`IFOURMAC`), `SONORA_CUDA`, then the modules below |
| `src/kernel.ld` | Links the hooks over `orwellsize` and `memcdjsize`, and the Cuda transport into the space of A/UX's Egret code |
| `src/toolbox.txt` | Byte patch to the Toolbox (`Patch.067C`): these Macs' BoxFlags |
| `bin/kernel.txt` | The kernel patch, for `/unix` and `/etc/config.d/newunix` |
| `../../modules/sonora/` | The Sonora board module (`sonora.S`, its patch sites in `sites.py`), and the Toolbox and sound driver changes all Sonora Macs take (`toolbox.txt`, `snd.txt`) |
| `../../modules/cuda/` | The Cuda transport for A/UX's Egret driver (`cuda.c`, `sites.py`) |
| `../../modules/common/hooks.S` | Hooks the board modules share, and the checkpoint code |

Notes: [docs/findings.md](../../docs/findings.md).
