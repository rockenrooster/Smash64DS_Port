	.arm
	.align 2
	.global ndsFreezeDiagnosticsIrqVector
	.type ndsFreezeDiagnosticsIrqVector, %function

ndsFreezeDiagnosticsIrqVector:
	sub lr, lr, #4
	stmdb sp!, {r0-r3, r12, lr}
	ldr r0, =gNdsFreezeDiagnosticsInterruptedPC
	str lr, [r0]
	ldr r0, =gNdsFreezeDiagnosticsInterruptedLR
	stmia r0, {lr}^
	ldr r0, [sp, #20]
	add lr, r0, #4
	ldr r0, =gNdsFreezeDiagnosticsOriginalIrqVector
	ldr r0, [r0]
	str r0, [sp, #20]
	ldmia sp!, {r0-r3, r12, pc}

	.size ndsFreezeDiagnosticsIrqVector, .-ndsFreezeDiagnosticsIrqVector
