#ifndef EDOS_IRQ_H
#define EDOS_IRQ_H
#include "memory.h"
int edos_irq_init(int enable_keyboard);
ed_u64 edos_timer_ticks(void);
#endif
