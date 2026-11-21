#ifndef EDOS_IRQ_H
#define EDOS_IRQ_H
#include "memory.h"
int edos_irq_init(int enable_keyboard);
ed_u64 edos_timer_ticks(void);
__attribute__((noreturn)) void edos_irq_resume_context(void *frame);
void *irq_dispatch(ed_u64 vector, void *frame);
#endif
