.syntax unified
.arch armv5te
.arm

.section .itcm.task9_float_phase2,"ax",%progbits
.align 2
.global __aeabi_fcmpeq
.hidden __aeabi_fcmpeq
.type __aeabi_fcmpeq,%function
__aeabi_fcmpeq:
    mov     r2, r0, lsl #1
    mov     r3, r1, lsl #1
    cmp     r2, #0xff000000
    cmpls   r3, #0xff000000
    teqls   r0, r1
    orrsne  ip, r2, r3
    moveq   r0, #1
    movne   r0, #0
    bx      lr
.size __aeabi_fcmpeq, .-__aeabi_fcmpeq

.section .note.GNU-stack,"",%progbits
