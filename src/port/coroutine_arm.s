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

