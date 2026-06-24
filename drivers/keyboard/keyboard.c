/*
 * AcharyaOS - keyboard.c
 * ----------------------
 * Interrupt-driven PS/2 set-1 keyboard driver. This first version handles
 * printable US-QWERTY keys, Enter, Backspace, Tab, and Shift. It intentionally
 * leaves extended keys, Caps Lock, Ctrl, and Alt for later shell/editor work.
 */

#include "keyboard.h"
#include "idt.h"
#include "pic.h"
#include "port_io.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_IRQ       1
#define KEY_RELEASED       0x80
#define KEYBUF_SIZE        128

static volatile char keybuf[KEYBUF_SIZE];
static volatile uint32_t keybuf_head = 0;
static volatile uint32_t keybuf_tail = 0;
static int shift_down = 0;

static const char scancode_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
    'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
};

static const char scancode_ascii_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"','~', 0, '|',
    'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ',
};

static void keyboard_buffer_push(char c) {
    uint32_t next_head = (keybuf_head + 1) % KEYBUF_SIZE;
    if (next_head == keybuf_tail) {
        return;
    }
    keybuf[keybuf_head] = c;
    keybuf_head = next_head;
}

static void keyboard_interrupt_handler(interrupt_frame_t *frame) {
    (void) frame;

    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    uint8_t key = (uint8_t)(scancode & ~KEY_RELEASED);
    int released = (scancode & KEY_RELEASED) != 0;

    if (key == 0x2A || key == 0x36) {
        shift_down = !released;
        pic_send_eoi(KEYBOARD_IRQ);
        return;
    }

    if (!released) {
        char c = shift_down ? scancode_ascii_shift[key] : scancode_ascii[key];
        if (c != 0) {
            keyboard_buffer_push(c);
        }
    }

    pic_send_eoi(KEYBOARD_IRQ);
}

void keyboard_init(void) {
    idt_register_handler(IDT_KEYBOARD_VECTOR, keyboard_interrupt_handler);
    pic_unmask_irq(KEYBOARD_IRQ);
}

int keyboard_getchar(void) {
    if (keybuf_head == keybuf_tail) {
        return -1;
    }

    char c = keybuf[keybuf_tail];
    keybuf_tail = (keybuf_tail + 1) % KEYBUF_SIZE;
    return (int) c;
}
