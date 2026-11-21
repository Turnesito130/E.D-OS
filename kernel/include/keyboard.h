#ifndef EDOS_KEYBOARD_H
#define EDOS_KEYBOARD_H
int edos_keyboard_available(void);
void edos_keyboard_init(void);
int edos_keyboard_poll_char(void);
int edos_keyboard_decode(unsigned char scancode, int shift, int caps_lock);
#endif
