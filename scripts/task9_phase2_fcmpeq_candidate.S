.syntax unified
.arch armv5te
.arm

.section .itcm.task9_fcmpeq,"ax",%progbits
.align 2
.global task9Phase2FcmpeqCandidate
.hidden task9Phase2FcmpeqCandidate
.type task9Phase2FcmpeqCandidate,%function
task9Phase2FcmpeqCandidate:
    mov     r2, r0, lsl #1
    mov     r3, r1, lsl #1
    cmp     r2, #0xff000000
    cmpls   r3, #0xff000000
    teqls   r0, r1
    orrsne  ip, r2, r3
    moveq   r0, #1
    movne   r0, #0
    bx      lr
.size task9Phase2FcmpeqCandidate, .-task9Phase2FcmpeqCandidate

.section .note.GNU-stack,"",%progbits
