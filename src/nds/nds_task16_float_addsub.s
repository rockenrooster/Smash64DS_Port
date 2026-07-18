	.syntax unified
	.arch armv5te
	.arm

	.section .itcm.task16_float_addsub,"ax",%progbits
	.align 2
	.global __aeabi_fsub
	.type __aeabi_fsub,%function
__aeabi_fsub:
	eor     r1, r1, #0x80000000
	.size __aeabi_fsub, .-__aeabi_fsub

	.global __aeabi_fadd
	.type __aeabi_fadd,%function
__aeabi_fadd:
	lsls    r2, r0, #1
	lslsne  r3, r1, #1
	teqne   r2, r3
	mvnsne  ip, r2, asr #24
	mvnsne  ip, r3, asr #24
	beq     .Lslow
	lsr     r2, r2, #24
	rsbs    r3, r2, r3, lsr #24
	addgt   r2, r2, r3
	eorgt   r1, r0, r1
	eorgt   r0, r1, r0
	eorgt   r1, r0, r1
	rsblt   r3, r3, #0
	cmp     r3, #25
	bxhi    lr
	tst     r0, #0x80000000
	orr     r0, r0, #0x00800000
	bic     r0, r0, #0xff000000
	rsbne   r0, r0, #0
	tst     r1, #0x80000000
	orr     r1, r1, #0x00800000
	bic     r1, r1, #0xff000000
	rsbne   r1, r1, #0
	teq     r2, r3
	beq     .Ldenormal_operand
.Lcombine:
	sub     r2, r2, #1
	adds    r0, r0, r1, asr r3
	rsb     r3, r3, #32
	lsl     r1, r1, r3
	and     r3, r0, #0x80000000
	bpl     .Labsolute
	rsbs    r1, r1, #0
	rsc     r0, r0, #0
.Labsolute:
	cmp     r0, #0x00800000
	bcc     .Lnormalize
	cmp     r0, #0x01000000
	bcc     .Lround
	lsrs    r0, r0, #1
	rrx     r1, r1
	add     r2, r2, #1
	cmp     r2, #254
	bcs     .Loverflow
.Lround:
	cmp     r1, #0x80000000
	adc     r0, r0, r2, lsl #23
	biceq   r0, r0, #1
	orr     r0, r0, r3
	bx      lr

.Lnormalize:
	lsls    r1, r1, #1
	adc     r0, r0, r0
	subs    r2, r2, #1
	cmpcs   r0, #0x00800000
	bcs     .Lround
	clz     ip, r0
	sub     ip, ip, #8
	lsl     r0, r0, ip
	subs    r2, r2, ip
	addge   r0, r0, r2, lsl #23
	rsblt   r2, r2, #0
	orrge   r0, r0, r3
	orrlt   r0, r3, r0, lsr r2
	bx      lr

.Ldenormal_operand:
	teq     r2, #0
	eor     r1, r1, #0x00800000
	eoreq   r0, r0, #0x00800000
	addeq   r2, r2, #1
	subne   r3, r3, #1
	b       .Lcombine

.Lslow:
	lsl     r3, r1, #1
	mvns    ip, r2, asr #24
	mvnsne  ip, r3, asr #24
	beq     .Lnan_or_infinity
	teq     r2, r3
	beq     .Lequal_magnitude
	teq     r2, #0
	moveq   r0, r1
	bx      lr

.Lequal_magnitude:
	teq     r0, r1
	movne   r0, #0
	bxne    lr
	tst     r2, #0xff000000
	bne     .Ldouble
	lsls    r0, r0, #1
	orrcs   r0, r0, #0x80000000
	bx      lr

.Ldouble:
	adds    r2, r2, #0x02000000
	addcc   r0, r0, #0x00800000
	bxcc    lr
	and     r3, r0, #0x80000000
.Loverflow:
	orr     r0, r3, #0x7f000000
	orr     r0, r0, #0x00800000
	bx      lr

.Lnan_or_infinity:
	mvns    r2, r2, asr #24
	movne   r0, r1
	mvnseq  r3, r3, asr #24
	movne   r1, r0
	lsls    r2, r0, #9
	lslseq  r3, r1, #9
	teqeq   r0, r1
	orrne   r0, r0, #0x00400000
	bx      lr
	.size __aeabi_fadd, .-__aeabi_fadd

	.section .note.GNU-stack,"",%progbits
