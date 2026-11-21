#include <assert.h>
#include <stdio.h>
#include "context.h"

static edos_cpu_context_t main_context, first_context, second_context;
static unsigned char first_stack[8192] __attribute__((aligned(16)));
static unsigned char second_stack[8192] __attribute__((aligned(16)));
static volatile int stage;
static void unexpected_return(void) { stage = -100; for (;;) {} }
static void first_task(void) {
    assert(stage == 0);
    stage = 1;
    edos_context_switch(&first_context, &main_context);
    assert(stage == 2);
    stage = 3;
    edos_context_switch(&first_context, &main_context);
    for (;;) {}
}
static void second_task(void) {
    assert(stage == 1);
    stage = 2;
    edos_context_switch(&second_context, &main_context);
    assert(stage == 3);
    stage = 4;
    edos_context_switch(&second_context, &main_context);
    for (;;) {}
}
int main(void) {
    assert(!edos_context_init(0, first_stack, sizeof(first_stack), first_task, unexpected_return));
    assert(!edos_context_init(&first_context, 0, sizeof(first_stack), first_task, unexpected_return));
    assert(edos_context_init(&first_context, first_stack, sizeof(first_stack), first_task, unexpected_return));
    assert(edos_context_init(&second_context, second_stack, sizeof(second_stack), second_task, unexpected_return));
    edos_context_switch(&main_context, &first_context);
    assert(stage == 1);
    edos_context_switch(&main_context, &second_context);
    assert(stage == 2);
    edos_context_switch(&main_context, &first_context);
    assert(stage == 3);
    edos_context_switch(&main_context, &second_context);
    assert(stage == 4);
    puts("CONTEXT_SWITCH_TESTS_OK");
    return 0;
}
