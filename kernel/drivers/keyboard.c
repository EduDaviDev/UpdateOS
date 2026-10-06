#include "keyboard.h"
#include "../io.h"
#include "../cpu/irq.h"
#include "../cpu/isr.h"
#include "../cpu/pic.h"

#define KBD_DATA_PORT   0x60
#define KBD_BUFFER_SIZE 256

/* Estado interno do driver */
static KBD_Event_t  event_buffer[KBD_BUFFER_SIZE];
static uint32_t     buf_head = 0;
static uint32_t     buf_tail = 0;

/* Rastreamento do estado de cada tecla.
 *
 * Índice:
 *   0x00..0x7F -> teclas normais  (base scancode)
 *   0x80..0xFF -> teclas estendidas (base scancode | 0x80)
 *
 * Valor: último estado conhecido (KS_NONE / KS_PRESSED / KS_RELEASED).
 * Começa tudo em KS_NONE; só passa a PRESSED após um press válido,
 * e só volta para RELEASED se houver um release coerente. */
static KBD_KeyState_t key_states[256];

/* Modificadores */
static bool shift_pressed = false;
static bool ctrl_pressed  = false;
static bool alt_pressed   = false;
static bool caps_lock     = false;

/* Máquina de estados para prefixos (E0/E1/F0 do Set 2) */
static bool     expect_extended = false;
static bool     expect_break    = false;
static bool     expect_e1       = false;
static uint8_t  e1_bytes[8];
static uint8_t  e1_count = 0;

/* Set ativo (padrão: Set 1, pois o 8042 geralmente traduz) */
static const KBD_ScanSet_t *active_set = &scancode_set1;

/* ---------------- Buffer ---------------- */

static bool buffer_push(const KBD_Event_t *ev)
{
    uint32_t next = (buf_head + 1) % KBD_BUFFER_SIZE;
    if (next == buf_tail) return false; /* cheio */
    event_buffer[buf_head] = *ev;
    buf_head = next;
    return true;
}

bool kbd_has_event = false;
bool kbd_initialized = false;

/* ---------------- Helpers ---------------- */

/* Retorna true se o scancode é break code no Set 1 (bit 7 setado) */
static inline bool is_break_set1(uint8_t sc)
{
    return (sc & 0x80) != 0;
}

/* Remove o bit de break do Set 1 */
static inline uint8_t strip_break_set1(uint8_t sc)
{
    return sc & 0x7F;
}

/* Índice na tabela de estados para uma tecla. */
static inline uint8_t key_track_index(uint8_t base, bool is_ext)
{
    return (uint8_t)(base | (is_ext ? 0x80u : 0x00u));
}

/* Atualiza a tabela de estados e devolve o estado resultante.
 *
 * Retorna KS_NONE quando o evento deve ser descartado — por exemplo,
 * release de uma tecla que não estava pressionada (spurious release),
 * evitando gerar KS_RELEASED sem um KS_PRESSED correspondente.
 *
 * Press sempre é aceito, inclusive auto-repeat (a tecla continua em
 * KS_PRESSED e o evento é repassado normalmente para permitir repetição
 * em editores de texto). */
static KBD_KeyState_t key_state_update(uint8_t track_idx, bool is_break)
{
    KBD_KeyState_t prev = key_states[track_idx];

    if (is_break) {
        if (prev != KS_PRESSED) {
            /* Release sem press -> ignora e mantém o estado atual. */
            return KS_NONE;
        }
        key_states[track_idx] = KS_RELEASED;
        return KS_RELEASED;
    }

    /* Press: sempre válido (inclui auto-repeat). */
    key_states[track_idx] = KS_PRESSED;
    return KS_PRESSED;
}

/* Preenche os filtros do evento a partir do caractere. */
static void kbd_fill_filters(KBD_Event_t *ev)
{
    char c = ev->character;

    if (c >= 0x20 && c < 0x7F) {
        ev->filters.printable = true;
        if (c >= '0' && c <= '9') ev->filters.numeric = true;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
            ev->filters.alphabetic = true;
        if (ev->filters.numeric || ev->filters.alphabetic)
            ev->filters.alphanumeric = true;
    } else {
        ev->filters.control = true;
    }
    if ((uint8_t)c >= 0x80) ev->filters.ascii_e = true;
}

/* ---------------- Handler da IRQ1 ---------------- */

static void kbd_irq_handler(registers_t *regs)
{
    (void)regs;
    uint8_t sc = inb(KBD_DATA_PORT);

    /* ---- Modo Set 2: tratar prefixos ---- */
    if (active_set == &scancode_set2) {
        if (sc == 0xE0) { expect_extended = true; return; }
        if (sc == 0xE1) { expect_e1 = true; e1_count = 0; return; }
        if (sc == 0xF0) { expect_break = true; return; }

        if (expect_e1) {
            /* Pause key: E1 1D 45 E1 9D C5 */
            if (e1_count < sizeof(e1_bytes)) e1_bytes[e1_count++] = sc;
            if (e1_count >= 6) {
                KBD_Event_t ev = {0};
                ev.keycode  = KEY_PAUSE;
                ev.state    = KS_PRESSED;
                ev.scancode = sc;
                buffer_push(&ev);
                kbd_has_event = true;
                e1_count = 0;
                expect_e1 = false;
            }
            return;
        }

        uint8_t base   = sc;
        bool    is_ext = expect_extended;
        bool    is_brk = expect_break;

        /* Limpa estados para o próximo byte */
        expect_extended = false;
        expect_break    = false;

        /* Extrai código e estado */
        KBD_KeyCodes_t keycode;
        char character = 0;

        if (is_ext) {
            keycode   = active_set->extmap[base];
            character = active_set->char_ext_map[base];
            if (shift_pressed) character = active_set->char_shift_ext_map[base];
        } else {
            keycode   = active_set->keymap[base];
            character = active_set->char_map[base];
            if (shift_pressed) character = active_set->char_shift_map[base];
        }

        /* Atualiza modificadores */
        if (!is_brk) {
            if (keycode == KEY_LSHIFT || keycode == KEY_RSHIFT) shift_pressed = true;
            if (keycode == KEY_LCTRL  || keycode == KEY_RCTRL)  ctrl_pressed  = true;
            if (keycode == KEY_LALT   || keycode == KEY_RALT)   alt_pressed   = true;
            if (keycode == KEY_CAPS)   caps_lock = !caps_lock;
        } else {
            if (keycode == KEY_LSHIFT || keycode == KEY_RSHIFT) shift_pressed = false;
            if (keycode == KEY_LCTRL  || keycode == KEY_RCTRL)  ctrl_pressed  = false;
            if (keycode == KEY_LALT   || keycode == KEY_RALT)   alt_pressed   = false;
        }

        /* Transição de estado — KS_NONE descarta o evento */
        uint8_t track = key_track_index(base, is_ext);
        KBD_KeyState_t state = key_state_update(track, is_brk);
        if (state == KS_NONE) return;

        KBD_Event_t ev = {0};
        ev.keycode   = keycode;
        ev.character = character;
        ev.scancode  = sc;
        ev.state     = state;
        ev.shift     = shift_pressed;
        ev.control   = ctrl_pressed;
        ev.alt       = alt_pressed;
        ev.sys       = false;

        kbd_fill_filters(&ev);

        if (buffer_push(&ev)) kbd_has_event = true;
        return;
    }

    /* ---- Modo Set 1 (padrão) ---- */
    bool    is_ext = (sc == 0xE0);

    if (is_ext) {
        /* Prefixo E0: próxima leitura é estendida */
        expect_extended = true;
        return;
    }

    bool    is_break = is_break_set1(sc);
    uint8_t base     = strip_break_set1(sc);

    /* Se ainda esperávamos um estendido, este byte é o código */
    bool was_ext = expect_extended;
    expect_extended = false;

    KBD_KeyCodes_t keycode;
    char character = 0;

    if (was_ext) {
        keycode   = active_set->extmap[base];
        character = active_set->char_ext_map[base];
        if (shift_pressed) character = active_set->char_shift_ext_map[base];
    } else {
        keycode   = active_set->keymap[base];
        character = active_set->char_map[base];
        if (shift_pressed) character = active_set->char_shift_map[base];
    }

    /* Atualiza modificadores */
    if (!is_break) {
        if (keycode == KEY_LSHIFT || keycode == KEY_RSHIFT) shift_pressed = true;
        if (keycode == KEY_LCTRL  || keycode == KEY_RCTRL)  ctrl_pressed  = true;
        if (keycode == KEY_LALT   || keycode == KEY_RALT)   alt_pressed   = true;
        if (keycode == KEY_CAPS)   caps_lock = !caps_lock;
    } else {
        if (keycode == KEY_LSHIFT || keycode == KEY_RSHIFT) shift_pressed = false;
        if (keycode == KEY_LCTRL  || keycode == KEY_RCTRL)  ctrl_pressed  = false;
        if (keycode == KEY_LALT   || keycode == KEY_RALT)   alt_pressed   = false;
    }

    /* Transição de estado — KS_NONE descarta o evento */
    uint8_t track = key_track_index(base, was_ext);
    KBD_KeyState_t state = key_state_update(track, is_break);
    if (state == KS_NONE) return;

    KBD_Event_t ev = {0};
    ev.keycode   = keycode;
    ev.character = character;
    ev.scancode  = sc;
    ev.state     = state;
    ev.shift     = shift_pressed;
    ev.control   = ctrl_pressed;
    ev.alt       = alt_pressed;
    ev.sys       = false;

    kbd_fill_filters(&ev);

    if (buffer_push(&ev)) kbd_has_event = true;
}

/* ---------------- API pública ---------------- */

void kbd_init(void)
{
    /* Registra handler no vetor 33 (IRQ1) */
    register_interrupt_handler(33, kbd_irq_handler);

    /* Desmascara IRQ1 no PIC */
    pic_clear_mask(1);

    /* Zera estado */
    buf_head = buf_tail = 0;

    for (uint32_t i = 0; i < sizeof(key_states) / sizeof(key_states[0]); i++)
        key_states[i] = KS_NONE;

    shift_pressed = ctrl_pressed = alt_pressed = false;
    caps_lock = false;
    expect_extended = expect_break = expect_e1 = false;
    e1_count = 0;
    kbd_has_event   = false;
    kbd_initialized = true;
}

KBD_Event_t kbd_gete(void)
{
    KBD_Event_t empty = {0};        /* state == KS_NONE (0) */
    if (buf_tail == buf_head) {
        kbd_has_event = false;
        return empty;
    }
    KBD_Event_t ev = event_buffer[buf_tail];
    buf_tail = (buf_tail + 1) % KBD_BUFFER_SIZE;
    if (buf_tail == buf_head) kbd_has_event = false;
    return ev;
}

char kbd_getc(void)
{
    /* Retorna o primeiro caractere imprimível disponível, sem remover o evento */
    uint32_t idx = buf_tail;
    while (idx != buf_head) {
        if (event_buffer[idx].filters.printable &&
            event_buffer[idx].state == KS_PRESSED &&
            event_buffer[idx].character != 0) {
            return event_buffer[idx].character;
        }
        idx = (idx + 1) % KBD_BUFFER_SIZE;
    }
    return 0;
}

uint8_t kbd_getsc(void)
{
    if (buf_tail == buf_head) return 0;
    return event_buffer[buf_tail].scancode;
}