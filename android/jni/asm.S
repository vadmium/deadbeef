#ifdef __ELF__
#   define ELF
#else
#   define ELF @
#endif

.macro  preserve8 val=1
ELF     .eabi_attribute 25, \val
.endm
.macro  function name, export=0
    .macro endfunc
ELF     .size   \name, . - \name
        .endfunc
        .purgem endfunc
    .endm
        .text
    .if \export
        .global EXTERN_ASM\name
EXTERN_ASM\name:
    .endif
ELF     .type   \name, %function
        .func   \name
\name:
.endm

