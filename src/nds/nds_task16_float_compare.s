	.syntax unified
	.arch armv5te
	.arm

	.section .itcm.task16_float_compare,"ax",%progbits
	.align 2

	.macro ordered_compare name, left, right, equal_result
	.global \name
	.type \name,%function
\name:
	mov     r2, \left, lsl #1
	mov     r3, \right, lsl #1
	cmp     r2, #0xff000000
	cmpls   r3, #0xff000000
	movhi   r0, #0
	bxhi    lr
	orrs    ip, r2, r3, lsr #1
	teqne   \left, \right
	subspl  r2, r2, r3
	mov     r0, \right, lsr #31
	eorcc   r0, r0, #1
	moveq   r0, #\equal_result
	bx      lr
	.size \name, .-\name
	.endm

	ordered_compare __aeabi_fcmplt, r0, r1, 0
	ordered_compare __aeabi_fcmple, r0, r1, 1
	ordered_compare __aeabi_fcmpge, r1, r0, 1
	ordered_compare __aeabi_fcmpgt, r1, r0, 0

	.global __aeabi_fcmpun
	.type __aeabi_fcmpun,%function
__aeabi_fcmpun:
	mov     r2, r0, lsl #1
	mov     r3, r1, lsl #1
	cmp     r2, #0xff000000
	cmpls   r3, #0xff000000
	movhi   r0, #1
	movls   r0, #0
	bx      lr
	.size __aeabi_fcmpun, .-__aeabi_fcmpun

	.section .note.GNU-stack,"",%progbits
