#include "keyboard.h"
#include "keydef.h"
#include "../libs/io.h"
#include "../interrupt/pic.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define KEYBOARD_BUFFER_SIZE 64

static volatile uint8_t buffer[KEYBOARD_BUFFER_SIZE];
static volatile int write_idx = 0;
static volatile int read_idx = 0;

static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool sys_pressed = false;
static bool extended = false;

static char char_buf[2];

void keyboard_init(void) {
    write_idx = 0;
    read_idx = 0;
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    sys_pressed = false;
    extended = false;
}

void keyboard_isr(void) {
    uint8_t sc = inb(0x60);

    if (sc == 0xE0) {
        extended = true;
        pic_send_eoi(1);
        return;
    }

    bool released = sc & 0x80;
    sc &= 0x7F;

    // Modificadores
    if (sc == 0x2A || sc == 0x36) { shift_pressed = !released; extended = false; pic_send_eoi(1); return; }
    if (sc == 0x1D) { ctrl_pressed = !released; extended = false; pic_send_eoi(1); return; }
    if (sc == 0x38) { alt_pressed = !released; extended = false; pic_send_eoi(1); return; }
    if ((sc == 0x5B || sc == 0x5C) && extended) { sys_pressed = !released; extended = false; pic_send_eoi(1); return; }

    // Se for release, ignoramos
    if (released) {
        extended = false;
        pic_send_eoi(1);
        return;
    }

    // Armazena o scancode no buffer linear
    int next = write_idx + 1;
    if (next - read_idx < KEYBOARD_BUFFER_SIZE) {
        buffer[write_idx % KEYBOARD_BUFFER_SIZE] = sc;
        write_idx = next;
    }

    extended = false;
    pic_send_eoi(1);
}

KeyEvent keyboard_read(void) {
    if (read_idx == write_idx) {
        KeyEvent ev = { .Char = NULL, .Code = KEY_NONE, .ScanCode = 0,
                        .Ctrl = false, .Shift = false, .Alt = false, .Sys = false,
                        .pressed = false, .released = false, .clicked = false };
        return ev;
    }

    uint8_t sc = buffer[read_idx % KEYBOARD_BUFFER_SIZE];
    read_idx++;

    KeyEvent ev;
    ev.ScanCode = sc;
    ev.Ctrl = ctrl_pressed;
    ev.Shift = shift_pressed;
    ev.Alt = alt_pressed;
    ev.Sys = sys_pressed;
    ev.pressed = true;
    ev.released = false;
    ev.clicked = false;

    const KeyCodes *key_map = (sc < sizeof(scset1_map)) ? scset1_map : scset1_ext_map;
    ev.Code = key_map[sc];

    char c = 0;
    if (sc < sizeof(scset1_normal_map)) {
        c = shift_pressed ? scset1_shift_map[sc] : scset1_normal_map[sc];
    }

    if (c == '\b' || c == '\t' || c == '\n' || (c >= 32 && c <= 126)) {
        char_buf[0] = c;
        char_buf[1] = '\0';
        ev.Char = char_buf;
    } else {
        ev.Char = NULL;
    }

    return ev;
}