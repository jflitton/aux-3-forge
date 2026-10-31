/*
 * setjmp and longjmp (setjmp.h). The jmp_buf: d2-d7 and a2-a6 at 0, the stack pointer
 * as setjmp found it at 44, the return address at 48, fp2-fp7 at 52; a sigjmp_buf then
 * has whether the mask was saved at 124 and the mask at 128.
 */
	.text

	.globl	setjmp, _setjmp
setjmp:
_setjmp:
	movea.l	4(%sp), %a0
	movem.l	%d2-%d7/%a2-%a6, (%a0)
	move.l	%sp, 44(%a0)
	move.l	(%sp), 48(%a0)
	fmovem.x %fp2-%fp7, 52(%a0)
	moveq	#0, %d0
	rts

	.globl	longjmp, _longjmp
longjmp:
_longjmp:
	movea.l	4(%sp), %a0
	move.l	8(%sp), %d0
	bne.s	1f
	moveq	#1, %d0			/* longjmp(env, 0) returns 1 */
1:	fmovem.x 52(%a0), %fp2-%fp7
	movem.l	(%a0), %d2-%d7/%a2-%a6
	movea.l	48(%a0), %a1
	movea.l	44(%a0), %sp
	addq.l	#4, %sp			/* as setjmp's rts left it */
	jmp	(%a1)

	.globl	sigsetjmp
sigsetjmp:
	movea.l	4(%sp), %a0
	move.l	8(%sp), 124(%a0)
	beq	setjmp
	clr.l	-(%sp)
	jsr	sigblock		/* sigblock(0): the mask */
	addq.l	#4, %sp
	movea.l	4(%sp), %a0
	move.l	%d0, 128(%a0)
	bra	setjmp

	.globl	siglongjmp
siglongjmp:
	movea.l	4(%sp), %a0
	tst.l	124(%a0)
	beq	longjmp
	move.l	128(%a0), -(%sp)
	jsr	sigsetmask
	addq.l	#4, %sp
	bra	longjmp
