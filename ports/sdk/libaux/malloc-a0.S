/*
 * malloc and realloc, for the shared library as well as for programs. A/UX's compiler
 * returns a pointer in a0 as well as d0, and the shared library, which calls these
 * through libc_malloc and libc_realloc, reads a0; gcc returns it in d0 only.
 */
	.text
	.globl	malloc
malloc:
	move.l	4(%sp), -(%sp)		/* n */
	jsr	__aux_malloc
	addq.l	#4, %sp
	move.l	%d0, %a0
	rts

	.globl	realloc
realloc:
	move.l	8(%sp), -(%sp)		/* n */
	move.l	8(%sp), -(%sp)		/* p */
	jsr	__aux_realloc
	addq.l	#8, %sp
	move.l	%d0, %a0
	rts
