/*
 * The lc520 group's kernel hooks: the LC 520, LC 550 and Color Classic II and their
 * Performa twins, Sonora Macs with Cuda, with the Sonora board module (modules/sonora) and
 * the Cuda transport (modules/cuda/cuda.c). src/kernel.ld places them.
 *
 * The machineID is Gestalt - 2: the LC 520 (and Performa 520) is Gestalt 56, the LC 550
 * (and Performa 550 and 560) 80, the Color Classic II (and Performa 275) 83. System Enabler
 * 403, which these Macs need for System 7.1, also lists Gestalt 81 (BoxFlag 0x4B), which no
 * source here names: perhaps a clock variant, as an LC III set to 33MHz reports itself as
 * an LC III+.
 */

#define SONORA_CUDA

LC520 = 0x36			/* Gestalt 56: LC 520, Performa 520 */
LC550 = 0x4e			/* Gestalt 80: LC 550, Performa 550, 560 */
GESTALT81 = 0x4f		/* Gestalt 81: listed by System Enabler 403 */
CCII = 0x51			/* Gestalt 83: Color Classic II, Performa 275 */

/* d0.w = machineID: branch to \yes on the Macs this group supports */
	.macro	IFOURMAC yes
	cmp.w	#LC520,%d0
	jbeq	\yes
	cmp.w	#LC550,%d0
	jbeq	\yes
	cmp.w	#GESTALT81,%d0
	jbeq	\yes
	cmp.w	#CCII,%d0
	jbeq	\yes
	.endm

#include "common/hooks.S"
#include "sonora/sonora.S"
