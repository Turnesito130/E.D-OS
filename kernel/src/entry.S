/* E.D-OS kernel entry. Limine transfers control in x86_64 long mode. */
.section .text.entry,"ax"
.global _start
.type _start, @function
.extern kernel_main

_start:
    cli
    cld
    lea kernel_stack_top(%rip), %rsp
    andq $-16, %rsp
    call kernel_main

1:
    cli
    hlt
    jmp 1b
.size _start, . - _start

.section .bss,"aw",@nobits
.align 16
kernel_stack_bottom:
    .skip 16384
.global kernel_stack_top
kernel_stack_top:

.section .note.GNU-stack,"",@progbits
