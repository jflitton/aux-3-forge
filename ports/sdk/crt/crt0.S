/*
 * Program entry. The kernel starts us at the first byte of .text (0xd0, after the
 * headers) with /shlib/libc1_s mapped, the stack holding argc, argv[0..argc-1], 0,
 * envp[...], 0, and d0 the stack limit, which we don't need. As A/UX's own crt1.o:
 * environ, then the shared library's pointers to the program's own data (__aux_init,
 * which A/UX's ld does with .init sections), then main and exit. argc, argv and envp
 * are kept in d2, a2 and a3 and pushed afresh for each call: a function may change the
 * arguments it was passed, and gcc's code does.
 */
	.section .text.start, "ax"
	.globl	_start
_start:
	move.l	(%sp), %d2		/* argc */
	lea	4(%sp), %a2		/* argv */
	lea	4(%a2, %d2.l*4), %a3	/* envp: past argv's terminating 0 */
	move.l	%a3, environ
	move.l	%a2, -(%sp)
	move.l	%d2, -(%sp)
	jsr	__aux_init
	addq.l	#8, %sp
	move.l	%a3, -(%sp)
	move.l	%a2, -(%sp)
	move.l	%d2, -(%sp)
	jsr	main
	move.l	%d0, (%sp)
	jsr	exit
