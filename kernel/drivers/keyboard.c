#include "keyboard.h"
#include "../io.h"
#include "../cpu/irq.h"
#include "../cpu/isr.h"

/* =========================================================
 *  Constantes de hardware (controlador 8042 PS/2)
 * ========================================================= */
#define KBD_DATA_PORT      0x60
#define KBD_STATUS_PORT    0x64
#define KBD_CMD_PORT       0x64

#define KBD_STATUS_OBF     0x01   /* Output Buffer Full */
#define KBD_STATUS_IBF     0x02   /* Input  Buffer Full */

#define KBD_CMD_READ_CFG   0x20
#define KBD_CMD_WRITE_CFG  0x60
#define KBD_CMD_ENABLE     0xAE

#define KBD_IRQ_NUM        1
#define KBD_IRQ_VECTOR     33     /* IRQ0 = 32, IRQ1 = 33 (PIC remapeado) */

#define KBD_BUFFER_SIZE    256

/* Pause (Set 2) = E1 1D 45 E1 9D C5.
 * O prefixo E1 já é consumido; restam 5 bytes a acumular. */
#define KBD_PAUSE_TAIL_LEN 5

/* =========================================================
 *  Estado interno do driver
 * ========================================================= */

/* Buffer circular de eventos (produtor: IRQ; consumidor: kernel) */
static KBD_Event_t        event_buffer[KBD_BUFFER_SIZE];
static volatile uint32_t  buf_head = 0;
static volatile uint32_t  buf_tail = 0;

/* Modificadores correntes */
static bool shift_pressed = false;
static bool ctrl_pressed  = false;
static bool alt_pressed   = false;
static bool caps_lock     = false;

/* Conjunto de scancodes ativo (Set 1 é o padrão; o 8042 traduz na maioria dos PCs) */
static const KBD_ScanSet_t *active_set = &scancode_set1;

/* Máquina de estados para prefixos */
static bool    ext_pending  = false;  /* vimos E0, o próximo byte é estendido */
static bool    brk_pending  = false;  /* vimos F0 (Set 2), o próximo é break   */
static bool    pause_active = false;  /* sequência de Pause em andamento       */
static uint8_t pause_buf[KBD_PAUSE_TAIL_LEN];
static uint8_t pause_len = 0;

/* Flags públicas (ver keyboard.h) */
volatile bool kbd_has_event   = false;
volatile bool kbd_initialized = false;

/* =========================================================
 *  Buffer circular
 * ========================================================= */

static inline bool buffer_is_full(void)
{
    return ((buf_head + 1) % KBD_BUFFER_SIZE) == buf_tail;
}

static inline bool buffer_is_empty(void)
{
    return buf_head == buf_tail;
}

static bool buffer_push(const KBD_Event_t *ev)
{
    if (buffer_is_full()) return false;
    event_buffer[buf_head] = *ev;
    buf_head = (buf_head + 1) % KBD_BUFFER_SIZE;
    kbd_has_event = true;
    return true;
}

static bool buffer_pop(KBD_Event_t *out)
{
    if (buffer_is_empty()) {
        kbd_has_event = false;
        return false;
    }
    *out = event_buffer[buf_tail];
    buf_tail = (buf_tail + 1) % KBD_BUFFER_SIZE;
    if (buffer_is_empty()) kbd_has_event = false;
    return true;
}

/* =========================================================
 *  Modificadores
 * ========================================================= */

static void modifiers_update(KBD_KeyCodes_t kc, bool is_break)
{
    switch (kc) {
    case KEY_LSHIFT: case KEY_RSHIFT: shift_pressed = !is_break; break;
    case KEY_LCTRL:  case KEY_RCTRL:  ctrl_pressed  = !is_break; break;
    case KEY_LALT:   case KEY_RALT:   alt_pressed   = !is_break; break;
    case KEY_CAPS:   if (!is_break)   caps_lock     = !caps_lock; break;
    default: break;
    }
}

/* =========================================================
 *  Construção de eventos
 * ========================================================= */

static void event_apply_filters(KBD_Event_t *ev)
{
    unsigned char c = (unsigned char)ev->character;

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

    if (c >= 0x80) ev->filters.ascii_e = true;
}

static KBD_Event_t event_make(KBD_KeyCodes_t kc, char ch,
                              uint8_t raw_sc, bool is_break)
{
    KBD_Event_t ev = {0};
    ev.keycode   = kc;
    ev.character = ch;
    ev.scancode  = raw_sc;
    ev.state     = is_break ? KS_RELEASED : KS_PRESSED;
    ev.shift     = shift_pressed;
    ev.control   = ctrl_pressed;
    ev.alt       = alt_pressed;
    ev.sys       = false;
    event_apply_filters(&ev);
    return ev;
}

/* =========================================================
 *  Lookup de keycode / caractere
 * ========================================================= */

static void lookup(const KBD_ScanSet_t *set, uint8_t base, bool is_ext,
                   KBD_KeyCodes_t *kc, char *ch)
{
    if (is_ext) {
        *kc = set->extmap[base];
        *ch = shift_pressed ? set->char_shift_ext_map[base]
                            : set->char_ext_map[base];
    } else {
        *kc = set->keymap[base];
        *ch = shift_pressed ? set->char_shift_map[base]
                            : set->char_map[base];
    }
}

/* =========================================================
 *  Máquinas de estado por scancode set
 * ========================================================= */

/* Ambas as funções retornam true quando um evento foi decodificado.
 * Prefixos (E0, F0, E1) são consumidos silenciosamente (retorno false). */

static bool decode_set1(uint8_t sc, KBD_KeyCodes_t *kc, char *ch,
                        bool *is_break, uint8_t *raw_sc)
{
    if (sc == 0xE0) {
        ext_pending = true;
        return false;
    }

    bool is_ext = ext_pending;
    ext_pending = false;

    bool    brk = (sc & 0x80) != 0;
    uint8_t b   = sc & 0x7F;

    lookup(active_set, b, is_ext, kc, ch);
    *is_break = brk;
    *raw_sc   = sc;
    return true;
}

static bool decode_set2(uint8_t sc, KBD_KeyCodes_t *kc, char *ch,
                        bool *is_break, uint8_t *raw_sc)
{
    /* Prefixos */
    if (sc == 0xE0) { ext_pending = true; return false; }
    if (sc == 0xF0) { brk_pending = true; return false; }
    if (sc == 0xE1) { pause_active = true; pause_len = 0; return false; }

    /* Acumula sequência de Pause */
    if (pause_active) {
        if (pause_len < KBD_PAUSE_TAIL_LEN) pause_buf[pause_len++] = sc;
        if (pause_len >= KBD_PAUSE_TAIL_LEN) {
            *kc       = KEY_PAUSE;
            *ch       = 0;
            *is_break = false;
            *raw_sc   = sc;
            pause_active = false;
            pause_len    = 0;
            return true;
        }
        return false;
    }

    bool is_ext = ext_pending;
    bool brk    = brk_pending;
    ext_pending = false;
    brk_pending = false;

    lookup(active_set, sc, is_ext, kc, ch);
    *is_break = brk;
    *raw_sc   = sc;
    return true;
}

/* =========================================================
 *  IRQ handler
 * ========================================================= */

static void kbd_irq_handler(registers_t *regs)
{
    (void)regs;

    uint8_t sc = inb(KBD_DATA_PORT);

    KBD_KeyCodes_t kc       = KEY_NONE;
    char           ch       = 0;
    bool           is_break = false;
    uint8_t        raw_sc   = sc;

    bool decoded = (active_set == &scancode_set1)
                       ? decode_set1(sc, &kc, &ch, &is_break, &raw_sc)
                       : decode_set2(sc, &kc, &ch, &is_break, &raw_sc);

    if (decoded) {
        modifiers_update(kc, is_break);
        KBD_Event_t ev = event_make(kc, ch, raw_sc, is_break);
        buffer_push(&ev);
    }

    pic_send_eoi(KBD_IRQ_NUM);
}

/* =========================================================
 *  API pública
 * ========================================================= */

void kbd_init(void)
{
    /* 1. Descarta qualquer byte pendente */
    while (inb(KBD_STATUS_PORT) & KBD_STATUS_OBF)
        (void)inb(KBD_DATA_PORT);

    /* 2. Habilita a interface do teclado (comando 0xAE) */
    while (inb(KBD_STATUS_PORT) & KBD_STATUS_IBF);
    outb(KBD_CMD_PORT, KBD_CMD_ENABLE);

    /* 3. Habilita IRQ1 no Configuration Byte do 8042 */
    while (inb(KBD_STATUS_PORT) & KBD_STATUS_IBF);
    outb(KBD_CMD_PORT, KBD_CMD_READ_CFG);
    while (!(inb(KBD_STATUS_PORT) & KBD_STATUS_OBF));
    uint8_t cfg = inb(KBD_DATA_PORT);
    cfg |= 0x01;                       /* IRQ1 enable */

    while (inb(KBD_STATUS_PORT) & KBD_STATUS_IBF);
    outb(KBD_CMD_PORT, KBD_CMD_WRITE_CFG);
    while (inb(KBD_STATUS_PORT) & KBD_STATUS_IBF);
    outb(KBD_DATA_PORT, cfg);

    /* 4. Registra handler no IDT e libera IRQ1 no PIC */
    register_interrupt_handler(KBD_IRQ_VECTOR, kbd_irq_handler);
    pic_clear_mask(KBD_IRQ_NUM);

    /* 5. Zera o estado do driver */
    buf_head = buf_tail = 0;

    shift_pressed = false;
    ctrl_pressed  = false;
    alt_pressed   = false;
    caps_lock     = false;

    ext_pending  = false;
    brk_pending  = false;
    pause_active = false;
    pause_len    = 0;

    kbd_has_event   = false;
    kbd_initialized = true;
}

KBD_Event_t kbd_gete(void)
{
    KBD_Event_t ev = {0};
    buffer_pop(&ev);           /* se vazio, devolve zero */
    return ev;
}

char kbd_getc(void)
{
    KBD_Event_t ev;
    while (buffer_pop(&ev)) {
        if (ev.filters.printable &&
            ev.state == KS_PRESSED &&
            ev.character != 0)
            return ev.character;
    }
    return 0;
}

uint8_t kbd_getsc(void)
{
    if (buffer_is_empty()) return 0;
    return event_buffer[buf_tail].scancode;
}
