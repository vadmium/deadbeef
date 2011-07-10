#include "asm.S"

        preserve8
        .fpu neon
        .text

# single iteration of the filter, auto-increments pointers
.macro apply_filter
# store xi: dhxi[band]=xi
    vst1.32 {q0}, [r0, :128]!
# load alpha
    vld1.32 {q1}, [r6, :128]!
# load beta
    vld1.32 {q2}, [r7, :128]!
# load gamma
    vld1.32 {q3}, [r8, :128]!
# load xk
    vld1.32 {q4}, [r1, :128]!
# load yj
    vld1.32 {q5}, [r3, :128]!
# load yk
    vld1.32 {q6}, [r4, :128]!

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
    vst1.32 {q4}, [r2, :128]!

# load gain[band]
    vld1.32 {q1}, [r9, :128]!
# yi *= gain
    vmul.f32 q4, q4, q1

# out += res
    vadd.f32 s31, s31, s16
    vadd.f32 s31, s31, s17
    vadd.f32 s31, s31, s18
    vadd.f32 s31, s31, s19
.endm

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

    apply_filter
    apply_filter
    apply_filter

# store out
    vmov.32         r0,  s31

# restore regs
    vpop {s31}
    pop {r4-r11}
    bx              lr
    
endfunc

