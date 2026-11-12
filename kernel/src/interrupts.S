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
    andq $-16, %rsp
    call kernel_exception_panic
1:
    cli
    hlt
    jmp 1b

.section .note.GNU-stack,"",@progbits
