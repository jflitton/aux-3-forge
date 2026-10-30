/*
 * Lays a program out as an A/UX 3 demand-paged executable (a.out magic 0413) that uses
 * the shared C library, which sdk/tools/elf2aout.py then writes as COFF:
 *
 *   - the file's first 0xd0 bytes are its headers (file, a.out, and four sections:
 *     .text, .data, .bss and .lib, which names the shared library); .text follows at
 *     the same address as its file offset, 0xd0, and starts with _start
 *   - .data follows .text in the file and is mapped at 0x400000 plus its file offset
 *     modulo 8KB, A/UX's page size
 *   - .bss follows .data
 *
 * The shared library's entry points are at fixed addresses, from libc1_s.ld.
 */
OUTPUT_FORMAT("elf32-m68k")
ENTRY(_start)
INCLUDE libc1_s.ld

SECTIONS
{
	. = 0xd0;
	.text : {
		*(.text.start)
		*(.text .text.*)
		*(.rodata .rodata.*)
		. = ALIGN(4);
	}
	etext = .;
	. = 0x400000 + (. & 0x1fff);
	.data : {
		*(.data .data.*)
		*(.sdata .sdata.*)
		. = ALIGN(4);
	}
	edata = .;
	.bss : {
		*(.bss .bss.*)
		*(.sbss .sbss.*)
		*(COMMON)
		. = ALIGN(4);
	}
	end = .;
	_end = .;
	/DISCARD/ : {
		*(.comment)
		*(.note .note.*)
		*(.eh_frame .eh_frame_hdr)
	}
}
