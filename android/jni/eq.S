#include "asm.S"

        preserve8
        .fpu neon
        .text

@ eq_apply_neon(dhxi,dhxk,dhyi,dhyj,dhyk,pcm)
# regs           r0   r1   r2   r3   r4   r5
function eq_apply_neon, export=1
# store regs
    push {r4-r11}
    vpush {s31}
    add r11, sp, #36
# load args from stack and globals
    ldmfd r11, {r4-r5}
    ldr r6, =iir_cfa
    ldr r7, =iir_cfb
    ldr r8, =iir_cfg
    ldr r9, =eq_gain

# duplicate pcm into q0 (xi)
    vdup.32 q0, r5

# init out
    mov r5, #0
    fmsr s31, r5

# current band offset in bytes, each band is 4 bytes, increments by 16 bytes
# every time
    mov r5, #0
loop:
# store xi: dhxi[band]=xi
    add r11, r0, r5
    vst1.32 {q0}, [r11, :128]
# load alpha
    add r11, r6, r5
    vld1.32 {q1}, [r11, :128]
# load beta
    add r11, r7, r5
    vld1.32 {q2}, [r11, :128]
# load gamma
    add r11, r8, r5
    vld1.32 {q3}, [r11, :128]
# load xk
    add r11, r1, r5
    vld1.32 {q4}, [r11, :128]
# load yj
    add r11, r3, r5
    vld1.32 {q5}, [r11, :128]
# load yk
    add r11, r4, r5
    vld1.32 {q6}, [r11, :128]

# from this point we can reuse registers r0,r1,r2,r3,r4

# perform calculation

# xi-xk = q0 - q4
    vsub.f32 q4, q0, q4
# alpha * (xi-xk) = q1 * q4
    vmul.f32 q4, q1, q4
# gamma * yj = q3 * q5
    vmul.f32 q3, q3, q5
# beta * yk = q2 * q6
    vmul.f32 q2, q2, q6
# alpha + gamma = q4 + q3
    vadd.f32 q4, q4, q3
# - beta = q0 - q2
    vsub.f32 q4, q4, q2

# store result (yi)
    add r11, r2, r5
    vst1.32 {q4}, [r11, :128]

# load gain[band]
    add r11, r9, r5
    vld1.32 {q1}, [r11, :128]
# yi *= gain
    vmul.f32 q4, q4, q1

# out += res
    vadd.f32 s31, s31, s16
    vadd.f32 s31, s31, s17
    vadd.f32 s31, s31, s18
    vadd.f32 s31, s31, s19

# loop
    add r5, r5, #16
    cmp r5, #48
    bne loop

# store out
    vmov.32         r0,  s31

# restore regs
    vpop {s31}
    pop {r4-r11}
    bx              lr
    
endfunc

