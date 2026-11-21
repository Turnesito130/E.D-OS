#include "keyboard.h"
typedef unsigned char u8;
typedef unsigned short u16;
static inline u8 inb(u16 port) {
    u8 value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
static int shift_down;
static int caps_lock;
#define KEY_QUEUE_CAPACITY 64U
static volatile u8 key_queue[KEY_QUEUE_CAPACITY];
static volatile unsigned int key_head;
static volatile unsigned int key_tail;
int edos_keyboard_decode(u8 code, int shift, int caps) {
    if (code & 0x80U) return 0;
    static const char normal[128] = {
        [0x02]='1',[0x03]='2',[0x04]='3',[0x05]='4',[0x06]='5',
        [0x07]='6',[0x08]='7',[0x09]='8',[0x0a]='9',[0x0b]='0',
        [0x0c]='-',[0x0d]='=',[0x0e]=8,[0x0f]=9,
        [0x10]='q',[0x11]='w',[0x12]='e',[0x13]='r',[0x14]='t',
        [0x15]='y',[0x16]='u',[0x17]='i',[0x18]='o',[0x19]='p',
        [0x1a]='[',[0x1b]=']',[0x1c]=10,
        [0x1e]='a',[0x1f]='s',[0x20]='d',[0x21]='f',[0x22]='g',
        [0x23]='h',[0x24]='j',[0x25]='k',[0x26]='l',
        [0x27]=';',[0x28]=0x27,[0x29]=0x60,[0x2b]=0x5c,
        [0x2c]='z',[0x2d]='x',[0x2e]='c',[0x2f]='v',[0x30]='b',
        [0x31]='n',[0x32]='m',[0x33]=',',[0x34]='.',
        [0x35]='/',[0x39]=' '
    };
    static const char shifted[128] = {
        [0x02]='!',[0x03]='@',[0x04]='#',[0x05]='$',[0x06]='%',
        [0x07]='^',[0x08]='&',[0x09]='*',[0x0a]='(',[0x0b]=')',
        [0x0c]='_',[0x0d]='+',[0x1a]='{',[0x1b]='}',
        [0x27]=':',[0x28]='"',[0x29]='~',[0x2b]='|',
        [0x33]='<',[0x34]='>',[0x35]='?'
    };
    char value = normal[code];
    if (!value) return 0;
    if (value >= 'a' && value <= 'z') {
        if ((shift != 0) != (caps != 0)) value = (char)(value - 'a' + 'A');
        return value;
    }
    return shift && shifted[code] ? shifted[code] : value;
}
int edos_keyboard_available(void) { return inb(0x64U) != 0xffU; }
int edos_keyboard_enqueue_scancode(u8 code) {
    unsigned int next = (key_tail + 1U) % KEY_QUEUE_CAPACITY;
    if (next == key_head) return 0;
    key_queue[key_tail] = code;
    key_tail = next;
    return 1;
}

void edos_keyboard_handle_irq(void) {
    if (inb(0x64U) & 1U) (void)edos_keyboard_enqueue_scancode(inb(0x60U));
}

void edos_keyboard_init(void) {
    shift_down = 0;
    caps_lock = 0;
    key_head = key_tail = 0;
    for (unsigned int i = 0; i < 32U && (inb(0x64U) & 1U); ++i)
        (void)inb(0x60U);
}
int edos_keyboard_poll_char(void) {
    while (key_head != key_tail) {
    u8 code = key_queue[key_head];
    key_head = (key_head + 1U) % KEY_QUEUE_CAPACITY;
    if (code == 0x2aU || code == 0x36U) { shift_down = 1; continue; }
    if (code == 0xaaU || code == 0xb6U) { shift_down = 0; continue; }
    if (code == 0x3aU) { caps_lock = !caps_lock; continue; }
    int value = edos_keyboard_decode(code, shift_down, caps_lock);
    if (value) return value;
    }
    return 0;
}
