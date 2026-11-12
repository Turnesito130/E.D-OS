/*
 * E.D-OS kernel entry stub.
 *
 * Contract: this entry point expects to be entered in 64-bit long mode by a
 * future, explicitly selected boot protocol/loader. No Linux kernel is used.
 * A bootloader and its hand-off contract are not implemented yet.
 */
.section .text.entry,"ax"
.global _start
.type _start, @function
.extern kernel_main

_start:
    cli
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
