#ifndef EDOS_IDT_H
#define EDOS_IDT_H

/* Installs a 256-entry IDT. Interrupts remain disabled until a PIC/APIC
 * controller and interrupt-routing policy have been implemented. */
int idt_init(void);

#endif
