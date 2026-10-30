#!/usr/bin/env python3
"""Write an m68k ELF executable, linked with sdk/lib/aux.ld, as an A/UX 3 COFF executable
that uses the shared C library.

  elf2aout.py IN.elf OUT

The output is what A/UX's own ld makes of a program, stripped: the COFF file header
(magic 0x150), the a.out header (magic 0413: demand paged), headers for .text, .data,
.bss and .lib, then the contents of .text, .data and .lib. .lib holds the shared
library's path, padded to 64 bytes, which tells the kernel to map /shlib/libc1_s into
the program. .text sits at its own file offset (0xd0) and .data at 0x400000 plus its
file offset modulo 8KB; sdk/lib/aux.ld lays the ELF out that way, and this checks it
did. .bss must follow .data, so if the program aligns .bss further, .data is padded with
zeros to meet it. No symbols, and a zero timestamp, so the same ELF always makes the same
file.

From aux-pkg's tools/elf2aout.py, which writes programs with no shared library.
"""
import struct, sys

HDRS = 20 + 28 + 4 * 40
SHLIB = b'/shlib/libc1_s'
F_RELFLG, F_EXEC, F_LNNO, F_LSYMS, F_AR32W = 0x1, 0x2, 0x4, 0x8, 0x200
STYP_TEXT, STYP_DATA, STYP_BSS, STYP_LIB = 0x20, 0x40, 0x80, 0x200
SHF_ALLOC, SHT_NOBITS = 0x2, 8


def die(msg):
    sys.exit('elf2aout: %s' % msg)


def sections(elf):
    if elf[:4] != b'\x7fELF' or elf[4] != 1 or elf[5] != 2:
        die('not a 32-bit big-endian ELF file')
    e_type, e_machine = struct.unpack('>HH', elf[16:20])
    if e_type != 2 or e_machine != 4:
        die('not an m68k executable')
    entry, = struct.unpack('>I', elf[24:28])
    shoff, = struct.unpack('>I', elf[32:36])
    shentsize, shnum, shstrndx = struct.unpack('>HHH', elf[46:52])
    shdrs = [struct.unpack('>IIIIIIIIII', elf[shoff + i * shentsize:
                                              shoff + i * shentsize + 40])
             for i in range(shnum)]
    strtab = shdrs[shstrndx]
    names = elf[strtab[4]:strtab[4] + strtab[5]]
    out = {}
    for name, typ, flags, addr, off, size, *_ in shdrs:
        name = names[name:names.index(b'\0', name)].decode()
        if not flags & SHF_ALLOC or not size:
            continue
        if name not in ('.text', '.data', '.bss'):
            die('unexpected section %s (only .text, .data and .bss can be loaded)' % name)
        out[name] = (addr, size, b'' if typ == SHT_NOBITS else elf[off:off + size])
    return entry, out


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__.strip().split('\n\n')[1])
    elf = open(sys.argv[1], 'rb').read()
    entry, secs = sections(elf)
    text = secs.get('.text')
    if not text or text[0] != HDRS:
        die('.text must start at %#x (link with sdk/lib/aux.ld)' % HDRS)
    tsize = len(text[2])
    data_off = HDRS + tsize
    data_start = 0x400000 + (data_off & 0x1fff)
    data = secs.get('.data', (data_start, 0, b''))
    bss = secs.get('.bss', (data[0] + data[1], 0, b''))
    if data[0] != data_start:
        die('.data is at %#x, not %#x' % (data[0], data_start))
    gap = bss[0] - (data[0] + data[1])
    if not 0 <= gap < 0x10000:
        die('.bss is at %#x, not right after .data (%#x)' % (bss[0], data[0] + data[1]))
    data = (data[0], data[1] + gap, data[2] + bytes(gap))
    if entry != HDRS:
        die('entry point %#x is not the start of .text' % entry)

    lib = SHLIB.ljust(64, b'\0')
    out = struct.pack('>HHiiiHH', 0x150, 4, 0, 0, 0, 28,
                      F_RELFLG | F_EXEC | F_LNNO | F_LSYMS | F_AR32W)
    out += struct.pack('>hhiiiiii', 0o413, 0, tsize, data[1], bss[1], entry, HDRS,
                       data_start)
    for name, (addr, size, _), ptr, flags in (
            ('.text', text, HDRS, STYP_TEXT),
            ('.data', data, data_off, STYP_DATA),
            ('.bss', bss, 0, STYP_BSS),
            ('.lib', (0, len(lib), lib), data_off + data[1], STYP_LIB)):
        out += struct.pack('>8siiiiiiHHi', name.encode(), addr, addr, size, ptr, 0, 0,
                           0, 0, flags)
    out += text[2] + data[2] + lib
    with open(sys.argv[2], 'wb') as f:
        f.write(out)


if __name__ == '__main__':
    main()
