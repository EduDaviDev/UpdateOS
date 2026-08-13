#include "keyboard.h"
#include "keydef.h"
#include "../libs/io.h"
#include "../interrupt/pic.h"
#include "../drivers/pit.h"   // para obter tempo do click
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define KEYBOARD_BUFFER_SIZE 64
#define CLICK_MIN_MS 300

static volatile uint8_t buffer[KEYBOARD_BUFFER_SIZE];
static volatile int write_idx = 0;
static volatile int read_idx = 0;

static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool sys_pressed = false;
static bool extended = false;

// Tabelas de mapeamento (mesmas de antes)
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

// Armazena o tempo do último pressionamento para calcular clicked
static uint32_t press_time = 0;

void kbd_init(void) {
    write_idx = 0;
    read_idx = 0;
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    sys_pressed = false;
    extended = false;
    press_time = 0;

    for (int i = 0; i < KEYBOARD_BUFFER_SIZE; i++) buffer[i] = 0;
    outb(0x64, 0xAE);   // habilita teclado
}

// Função auxiliar para preencher filtros
static void fill_filters(KeyEvent *ev) {
    char c = ev->character;
    ev->filter.nschar = (c >= 32 && c <= 126);  // imprimível
    ev->filter.numchar = (c >= '0' && c <= '9');
    ev->filter.hexchar = ((c >= '0' && c <= '9') ||
                          (c >= 'A' && c <= 'F') ||
                          (c >= 'a' && c <= 'f'));
    // Especiais: !@#$%^&*()_+-=[]{};:'",.<>/?\|`~
    ev->filter.spchar = (c >= 33 && c <= 47) ||
                        (c >= 58 && c <= 64) ||
                        (c >= 91 && c <= 96) ||
                        (c >= 123 && c <= 126);
}

// ISR do teclado (mesma de antes, mas com nome keyboard_isr)
void keyboard_isr(void) {
    uint8_t sc = inb(0x60);

    if (sc == 0xE0) {
        extended = true;
        pic_send_eoi(1);
        return;
    }

    bool released = sc & 0x80;
    sc &= 0x7F;

    if (sc == 0x2A || sc == 0x36) { shift_pressed = !released; extended = false; pic_send_eoi(1); return; }
    if (sc == 0x1D) { ctrl_pressed = !released; extended = false; pic_send_eoi(1); return; }
    if (sc == 0x38) { alt_pressed = !released; extended = false; pic_send_eoi(1); return; }
    if ((sc == 0x5B || sc == 0x5C) && extended) { sys_pressed = !released; extended = false; pic_send_eoi(1); return; }

    if (released) {
        extended = false;
        pic_send_eoi(1);
        return;
    }

    int next = write_idx + 1;
    if (next - read_idx < KEYBOARD_BUFFER_SIZE) {
        buffer[write_idx % KEYBOARD_BUFFER_SIZE] = sc;
        write_idx = next;
    }

    extended = false;
    pic_send_eoi(1);
}

// Função para construir o evento a partir do scancode
static KeyEvent build_event(uint8_t sc) {
    KeyEvent ev;
    ev.scancode = sc;
    ev.mods.ctrl = ctrl_pressed;
    ev.mods.shift = shift_pressed;
    ev.mods.alt = alt_pressed;
    ev.mods.sys = sys_pressed;

    const KeyCodes *key_map = (sc < sizeof(scset1_map)) ? scset1_map : scset1_ext_map;
    ev.keycode = key_map[sc];

    char c = 0;
    if (sc < 128) {
        c = shift_pressed ? shift_map[sc] : normal_map[sc];
    }
    ev.character = c;
    fill_filters(&ev);
    ev.pressed = true;
    ev.released = false;
    ev.clicked = false;
    ev.extended = false; // já foi tratado

    // Registrar tempo de pressionamento (para click) - se não for caractere especial (setas etc.)
    // Vamos usar o PIT para contar ticks em ms
    if (c != 0) {
        press_time = pit_get_ticks_ms();  // precisamos implementar pit_get_ticks_ms()
    }

    return ev;
}

// Obtém evento (não bloqueante)
KeyEvent kbd_gevent(void) {
    if (read_idx == write_idx) {
        KeyEvent ev = { .character = '\0', .scancode = 0, .keycode = KEY_NONE,
                        .mods = {false, false, false, false},
                        .filter = {false, false, false, false},
                        .pressed = false, .released = false, .clicked = false,
                        .extended = false };
        return ev;
    }

    uint8_t sc = buffer[read_idx % KEYBOARD_BUFFER_SIZE];
    read_idx++;
    return build_event(sc);
}

// Obtém evento com output (bloqueante até tecla)
KeyEvent kbd_ogevent(void) {
    KeyEvent ev;
    do {
        ev = kbd_gevent();
        // Se não há tecla, faz uma pausa curta (usando PIT ou loop)
        if (!ev.pressed && !ev.released) {
            // Espera um pouco (implementar com pit_wait_ms ou busy loop)
            // Aqui usaremos um loop simples (não ideal, mas funcional)
            for (volatile int i = 0; i < 10000; i++);
        }
    } while (!ev.pressed && !ev.released);
    return ev;
}