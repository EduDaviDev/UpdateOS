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

// Tabela de mapeamento direta: scancode (0-127) -> caractere normal
static const char normal_map[128] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,   0,   'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,   '\\','z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

// Tabela com Shift pressionado
static const char shift_map[128] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,   'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

void keyboard_init(void) {
    write_idx = 0;
    read_idx = 0;
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    sys_pressed = false;
    extended = false;

    // Limpa o buffer (garante que não haja lixo)
    for (int i = 0; i < KEYBOARD_BUFFER_SIZE; i++) buffer[i] = 0;

    // Ativa o teclado (se necessário)
    outb(0x64, 0xAE);   // habilita o teclado (comando para o controlador)
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

    // Armazena o scancode no buffer circular
    int next = write_idx + 1;
    if (next - read_idx < KEYBOARD_BUFFER_SIZE) {
        buffer[write_idx % KEYBOARD_BUFFER_SIZE] = sc;
        write_idx = next;
    }

    extended = false;
    pic_send_eoi(1);
}

KeyEvent keyboard_read(void) {
    // Se não há tecla, retorna evento vazio
    if (read_idx == write_idx) {
        KeyEvent ev = { .Char = '\0', .Code = KEY_NONE, .ScanCode = 0,
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

    // Mapeia o código da tecla (para KEY_RETURN, KEY_BACKSPACE, etc.)
    const KeyCodes *key_map = (sc < sizeof(scset1_map)) ? scset1_map : scset1_ext_map;
    ev.Code = key_map[sc];

    // Mapeia o caractere usando as tabelas simples
    char c = 0;
    if (sc < 128) {
        c = shift_pressed ? shift_map[sc] : normal_map[sc];
    }
    ev.Char = c;   // se não mapeado, '\0'

    return ev;
}