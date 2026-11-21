#include <assert.h>
#include <stdio.h>
#include "keyboard.h"
int main(void) {
    assert(edos_keyboard_decode(0x1e, 0, 0) == 'a');
    assert(edos_keyboard_decode(0x1e, 1, 0) == 'A');
    assert(edos_keyboard_decode(0x1e, 0, 1) == 'A');
    assert(edos_keyboard_decode(0x1e, 1, 1) == 'a');
    assert(edos_keyboard_decode(0x02, 0, 0) == '1');
    assert(edos_keyboard_decode(0x02, 1, 0) == '!');
    assert(edos_keyboard_decode(0x1c, 0, 0) == 10);
    assert(edos_keyboard_decode(0x39, 0, 0) == ' ');
    assert(edos_keyboard_decode(0x9e, 0, 0) == 0);
    assert(edos_keyboard_decode(0x7f, 0, 0) == 0);
    puts("KEYBOARD_TESTS_OK");
    return 0;
}
