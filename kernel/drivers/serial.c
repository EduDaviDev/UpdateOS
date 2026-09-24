#include "serial.h"
#include "../io.h"   /* se você já tem io.h com inb/outb, use-o */

/* ------------------------------------------------------------------ */
/*  Registradores da UART 16550 (offset a partir da base)              */
/* ------------------------------------------------------------------ */
#define UART_DATA         0   /* DLAB=0: data | DLAB=1: divisor low   */
#define UART_IER          1   /* DLAB=0: int enable | DLAB=1: div hi */
#define UART_FIFO         2   /* FIFO control                          */
#define UART_LCR          3   /* Line control                          */
#define UART_MCR          4   /* Modem control                         */
#define UART_LSR          5   /* Line status                           */
#define UART_MSR          6   /* Modem status                          */
#define UART_SCRATCH      7

/* Bits do LSR (Line Status Register) */
#define LSR_DATA_READY    0x01
#define LSR_THR_EMPTY     0x20

static uint16_t g_com_base = SERIAL_COM1;

/* ------------------------------------------------------------------ */
int serial_init(void) {
    uint16_t b = g_com_base;

    outb(b + UART_IER, 0x00);   /* desabilita interrupções            */
    outb(b + UART_LCR, 0x80);   /* habilita DLAB para setar baud rate */
    outb(b + UART_DATA, 0x03);  /* divisor low  = 3 → 38400 baud      */
    outb(b + UART_IER,  0x00);  /* divisor high = 0                    */
    outb(b + UART_LCR, 0x03);   /* 8 bits, sem paridade, 1 stop (8N1)  */
    outb(b + UART_FIFO, 0xC7);  /* habilita FIFO, limpa, threshold 14  */
    outb(b + UART_MCR,  0x0B);  /* DTR, RTS, OUT2                      */

    /* Loopback para testar */
    outb(b + UART_MCR, 0x1E);   /* loopback mode                       */
    outb(b + UART_DATA, 0xAE);  /* envia byte de teste                 */
    if (inb(b + UART_DATA) != 0xAE) {
        return 0;               /* UART não respondeu                  */
    }

    outb(b + UART_MCR, 0x0B);   /* modo normal                         */
    return 1;
}

/* ------------------------------------------------------------------ */
int serial_is_transmit_empty(void) {
    return inb(g_com_base + UART_LSR) & LSR_THR_EMPTY;
}

int serial_received(void) {
    return inb(g_com_base + UART_LSR) & LSR_DATA_READY;
}

void serial_putc(char c) {
    while (!serial_is_transmit_empty()) { /* espera */ }
    outb(g_com_base + UART_DATA, (uint8_t)c);
}

char serial_getc(void) {
    while (!serial_received()) { /* espera */ }
    return (char)inb(g_com_base + UART_DATA);
}

/* ------------------------------------------------------------------ */
void serial_print(const char *s) {
    while (*s) {
        if (*s == '\n') serial_putc('\r');   /* CRLF para terminais */
        serial_putc(*s++);
    }
}

void serial_write(const char *s, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\n') serial_putc('\r');
        serial_putc(s[i]);
    }
}

/* ------------------------------------------------------------------ */
void serial_print_dec(uint32_t v) {
    char buf[11];
    int i = 0;
    if (v == 0) { serial_putc('0'); return; }
    while (v > 0 && i < 10) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i-- > 0) serial_putc(buf[i]);
}

void serial_print_hex32(uint32_t v) {
    static const char hex[] = "0123456789ABCDEF";
    serial_print("0x");
    for (int i = 28; i >= 0; i -= 4) serial_putc(hex[(v >> i) & 0xF]);
}

void serial_print_hex64(uint64_t v) {
    serial_print_hex32((uint32_t)(v >> 32));
    serial_print_hex32((uint32_t)(v & 0xFFFFFFFFu));
}

/* ------------------------------------------------------------------ */
/*  Dump de registradores em caso de exception.                        */
/*  Chame isso dentro do handler de #PF no isr.c / idt.c               */
/* ------------------------------------------------------------------ */
void serial_dump_regs(uint32_t eip, uint32_t cr2, uint32_t err) {
    serial_print("\n*** EXCEPTION ***\n");
    serial_print("  EIP = "); serial_print_hex32(eip); serial_putc('\n');
    serial_print("  CR2 = "); serial_print_hex32(cr2); serial_putc('\n');
    serial_print("  ERR = "); serial_print_hex32(err); serial_putc('\n');
    serial_print("*** KERNEL PANIC ***\n");
}