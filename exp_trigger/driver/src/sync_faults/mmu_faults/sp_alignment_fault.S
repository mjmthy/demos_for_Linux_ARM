#include <linux/linkage.h>
#include <asm/insn.h>

.global sp_alignment_fault
sp_alignment_fault:
	mov 	x10, sp
	add	x10, x10, #0x1
	mov	sp, x10
	/*
	 * access unaligned sp as
	 * base address can trigger
	 * SP alignment fault
	 */
	ldr	x11, [sp]
	ret
