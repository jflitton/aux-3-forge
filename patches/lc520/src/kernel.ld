/* Link the Sonora module's hooks (src/lc520.S with modules/sonora) over orwellsize and
   memcdjsize, up to _DAFBMonitorDetect, as for the lc3 group; and the Cuda transport
   (modules/cuda/cuda.c) over cEgretDispatch and the Egret helpers it replaces, after the
   6-byte jmp sonora_cegret that kernel-patch.py puts at cEgretDispatch, so every request
   passes the DFAC filter first. cuda_dispatch must come first in .text2. */
SECTIONS
{
	/* first, so that .text2's wildcard does not take it */
	.text orwellsize : { *(.text.sonora) }
	.text2 cEgretDispatch + 6 : {
		*(.text.cuda_dispatch)
		*(.text .text.*)
		*(.rodata .rodata.*)
	}
	.data : { *(.data .data.*) *(.bss .bss.* COMMON) }
	/DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
ASSERT(SIZEOF(.data) == 0, "the hooks and cuda.c must not use static data")
ASSERT(memcdjsize > orwellsize, "memcdjsize no longer follows orwellsize")
ASSERT(SIZEOF(.text) <= _DAFBMonitorDetect - orwellsize, "hooks overrun _DAFBMonitorDetect")
ASSERT(SIZEOF(.text2) <= shift_reg_intr - cEgretDispatch - 6, "Cuda transport overruns shift_reg_intr")
