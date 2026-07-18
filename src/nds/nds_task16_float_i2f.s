	.syntax unified
	.arch armv5te
	.arm

	.section .itcm.task16_i2f,"ax",%progbits
	.align 2
	.global __aeabi_i2f
	.hidden __aeabi_i2f
	.type __aeabi_i2f,%function
__aeabi_i2f:
	cmp     r0, #0
	bxeq    lr
	and     r3, r0, #0x80000000
	rsbmi   r0, r0, #0
	clz     r1, r0
	rsb     r1, r1, #31
	cmp     r1, #23
	bls     .Li2f_exact

	sub     r2, r1, #23
	mov     ip, r0, lsr r2
	rsb     r2, r2, #32
	mov     r2, r0, lsl r2
	and     r0, ip, #1
	orr     r2, r2, r0
	movs    r2, r2, lsl #1
	addhi   ip, ip, #1
	b       .Li2f_pack

.Li2f_exact:
	rsb     r2, r1, #23
	mov     ip, r0, lsl r2

.Li2f_pack:
	add     r1, r1, #126
	add     r0, ip, r1, lsl #23
	orr     r0, r0, r3
	bx      lr
	.size __aeabi_i2f, .-__aeabi_i2f

	.section .note.GNU-stack,"",%progbits
