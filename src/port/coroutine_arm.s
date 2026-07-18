    .syntax unified
    .arm
    .text
    .align 2

    .global ndsCoroutineSwap
    .type ndsCoroutineSwap, %function
ndsCoroutineSwap:
    stmia   r0, {r4-r11}
    str     sp, [r0, #32]
    str     lr, [r0, #36]

    ldmia   r1, {r4-r11}
    ldr     sp, [r1, #32]
    ldr     lr, [r1, #36]
    bx      lr
    .size ndsCoroutineSwap, .-ndsCoroutineSwap

    .global ndsCoroutineTrampoline
    .type ndsCoroutineTrampoline, %function
ndsCoroutineTrampoline:
    mov     r0, r4
    bl      portCoroutineTrampolineC
1:
    b       1b
    .size ndsCoroutineTrampoline, .-ndsCoroutineTrampoline

    /* Kept separate so --gc-sections removes Task 20 from normal ROMs. */
    .section .text.ndsCoroutineTask20StackProfile, "ax", %progbits
    .align 2
    .arm
    .global ndsCoroutineReadSp
    .type ndsCoroutineReadSp, %function
ndsCoroutineReadSp:
    mov     r0, sp
    bx      lr
    .size ndsCoroutineReadSp, .-ndsCoroutineReadSp

    .global ndsCoroutinePoisonWords
    .type ndsCoroutinePoisonWords, %function
ndsCoroutinePoisonWords:
1:
    cmp     r0, r1
    bhs     2f
    str     r2, [r0], #4
    b       1b
2:
    bx      lr
    .size ndsCoroutinePoisonWords, .-ndsCoroutinePoisonWords
