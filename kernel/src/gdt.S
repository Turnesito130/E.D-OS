.section .text
.global edos_gdt_load
.type edos_gdt_load, @function
edos_gdt_load:
    lgdt (%rdi)
    pushq $0x08
    leaq 1f(%rip), %rax
    pushq %rax
    lretq
1:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    xorw %ax, %ax
    movw %ax, %fs
    movw %ax, %gs
    movw $0x28, %ax
    ltr %ax
    ret
.size edos_gdt_load, . - edos_gdt_load
.section .note.GNU-stack,"",@progbits
