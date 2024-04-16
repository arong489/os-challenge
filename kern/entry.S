#include <asm/asm.h>
#include <stackframe.h>

.section .text.tlb_miss_entry
tlb_miss_entry:
	# j       exc_gen_entry
	# lab 2 challenge
	li		k1, KSTACKTOP
	mfc0    k0, CP0_ENTRYHI
	sw      k0, 0(k1)
	mfc0    k0, CP0_EPC
	sw      k0, 4(k1)
	sw		s0, 8(k1)

	mfc0 	s0, CP0_CONTEXT		# 虽然SAVE_ALL只改变k0
	lw 		s0, 0(s0)			# 但k0, k1寄存器不安全
								# here, it is likely to be trap into kern tlb miss

	mtc0	s0, CP0_ENTRYLO0
	li		k0, KSTACKTOP
	lw      s0, 8(k0)
	lw		k1, 0(k0)
	mtc0    k1, CP0_ENTRYHI
	lw      k0, 4(k0)
	nop

	tlbwr
	jr 		k0
	rfe


.section .text.exc_gen_entry
exc_gen_entry:
	SAVE_ALL
	/* Exercise 3.9: Your code here.*/
	mfc0	t0, CP0_CAUSE
	andi	t0, 0x7c
	lw	t0, exception_handlers(t0)
	jr	t0