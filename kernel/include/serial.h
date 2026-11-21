#ifndef EDOS_SERIAL_H
#define EDOS_SERIAL_H
#include <stddef.h>
int edos_serial_init(void);
int edos_serial_write_char(char value);
size_t edos_serial_write(const char *text);
#endif
