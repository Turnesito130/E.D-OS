/* Exception entry stubs. The common path is deliberately non-returning:
 * E.D-OS records the fault and halts rather than attempting unsafe recovery. */
.section .text
.extern kernel_exception_panic

.macro ISR_NOERR number
.global isr\number
.type isr\number, @function
isr\number:
    pushq $0
    pushq $\number
    jmp isr_common
.endm

.macro ISR_ERR number
.global isr\number
.type isr\number, @function
isr\number:
    pushq $\number
    jmp isr_common
.endm

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR   29
ISR_ERR   30
ISR_NOERR 31

.global isr_unexpected
.type isr_unexpected, @function
isr_unexpected:
    pushq $0
    pushq $255
    jmp isr_common

.type isr_common, @function
isr_common:
    cli
    movq 0(%rsp), %rdi
    movq 8(%rsp), %rsi
    movq 16(%rsp), %rdx
    andq $-16, %rsp
    call kernel_exception_panic
1:
    cli
    hlt
    jmp 1b

 .global irq_keyboard_stub
.type irq_keyboard_stub, @function
irq_keyboard_stub:
    pushq $0
    pushq $33
    jmp irq_common

.global irq_timer_stub
.type irq_timer_stub, @function
irq_timer_stub:
    pushq $0
    pushq $32
    jmp irq_common

.type irq_common, @function
irq_common:
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
    movq 120(%rbx), %rdi
    movq %rbx, %rsi
    andq $-16, %rsp
    call irq_dispatch
    movq %rax, %rsp
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

.global edos_irq_resume_context
.type edos_irq_resume_context, @function
edos_irq_resume_context:
    movq %rdi, %rsp
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

.section .note.GNU-stack,"",@progbits
