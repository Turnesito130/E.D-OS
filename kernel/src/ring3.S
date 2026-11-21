.section .text
.extern kernel_syscall_entry

.global syscall_stub
.type syscall_stub, @function
syscall_stub:
    pushq $0
    pushq $128
    jmp syscall_common

.type syscall_common, @function
syscall_common:
    cld
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp, %rbx
    movq 112(%rbx), %rdi
    movq 72(%rbx), %rsi
    movq 80(%rbx), %rdx
    andq $-16, %rsp
    call kernel_syscall_entry
    movq %rax, 112(%rbx)
    movq %rbx, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    addq $16, %rsp
    iretq
.size syscall_stub, . - syscall_stub

.global edos_enter_user_mode
.type edos_enter_user_mode, @function
edos_enter_user_mode:
    movw $0x1b, %ax
    movw %ax, %ds
    movw %ax, %es
    pushq $0x1b
    pushq %rsi
    pushq $0x202
    pushq $0x23
    pushq %rdi
    iretq
.size edos_enter_user_mode, . - edos_enter_user_mode

.section .rodata
.align 16
.global edos_ring3_blob_start
.global edos_ring3_blob_end
edos_ring3_blob_start:
    movq $3, %rax
    int $0x80
    cmpq $42, %rax
    jne 1f
    movabsq $0x0000400000001000, %rdi
    movq $16, %rsi
    xorq %rax, %rax
    int $0x80
    movq $1, %rax
    xorq %rdi, %rdi
    int $0x80
    ud2
1:
    movq $1, %rax
    movq $1, %rdi
    int $0x80
    ud2
edos_ring3_blob_end:

.section .note.GNU-stack,"",@progbits
