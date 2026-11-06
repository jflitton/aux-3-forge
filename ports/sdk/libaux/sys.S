/*
 * System calls without an entry in the shared library in every A/UX 3 version, as
 * pkg/rt/sys.S makes them. BSD-style calls (trap #15) take the call number in d0
 * and arguments in a0, d1, a1, d2, a2, d3; the result comes back in d0, or carry set
 * and the error number in d0.
 */
	.text

	/* 3.0.1's gettimeofday, which 3.1 keeps beside its own; it takes no timezone */
	.globl	__aux_gettimeofday
__aux_gettimeofday:
	movea.l	4(%sp), %a0
	move.l	#0x88, %d0
	trap	#15
	bcs.s	1f
	moveq	#0, %d0
	rts
1:	move.l	%d0, errno
	moveq	#-1, %d0
	rts
