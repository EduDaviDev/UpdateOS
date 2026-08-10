#include "serial.h"
#include "../libs/io.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

// ============================================================
// Definições da porta serial COM1
// ============================================================

#define COM1_BASE 0x3F8

#define SERIAL_DATA         (COM1_BASE + 0)
#define SERIAL_IER          (COM1_BASE + 1)   // Interrupt Enable
#define SERIAL_FCR          (COM1_BASE + 2)   // FIFO Control
#define SERIAL_LCR          (COM1_BASE + 3)   // Line Control
#define SERIAL_MCR          (COM1_BASE + 4)   // Modem Control
#define SERIAL_LSR          (COM1_BASE + 5)   // Line Status
#define SERIAL_MSR          (COM1_BASE + 6)   // Modem Status
#define SERIAL_SCR          (COM1_BASE + 7)   // Scratch

#define SERIAL_LSR_THRE     0x20              // Transmitter Holding Register Empty
#define SERIAL_LSR_TEMT     0x40              // Transmitter Empty

static bool initialized = false;

// ============================================================
// Inicialização
// ============================================================

void serial_init(void) {
    if (initialized) return;

    // Desabilita interrupções
    outb(SERIAL_IER, 0x00);

    // Ativa DLAB (Divisor Latch Access Bit) para configurar baud rate
    outb(SERIAL_LCR, 0x80);

    // Divisor = 115200 / 9600 = 12 (0x000C)
    outb(SERIAL_DATA, 0x0C);   // LSB
    outb(SERIAL_IER,  0x00);   // MSB

    // Configura linha: 8 bits, 1 stop bit, sem paridade (0x03)
    outb(SERIAL_LCR, 0x03);

    // Ativa FIFO, limpa buffers, configura para 14 bytes de threshold
    outb(SERIAL_FCR, 0xC7);

    // Configura DTR e RTS ativos (para modem)
    outb(SERIAL_MCR, 0x03);

    // Testa se a porta responde (escreve e lê scratch)
    outb(SERIAL_SCR, 0x55);
    if (inb(SERIAL_SCR) != 0x55) {
        // Porta não disponível – mas continuamos de qualquer forma
    }

    initialized = true;
}

// ============================================================
// Envio de caractere (polling)
// ============================================================

static void serial_wait_transmit(void) {
    uint32_t timeout = 1000000;
    while (timeout--) {
        if (inb(SERIAL_LSR) & SERIAL_LSR_THRE) {
            return;
        }
    }
    // Se timeout, continua mesmo assim (evita travamento infinito)
}

void serial_putc(char c) {
    if (!initialized) serial_init();
    // Se for '\n', envia também '\r' (para compatibilidade com terminais)
    if (c == '\n') {
        serial_putc('\r');
    }
    serial_wait_transmit();
    outb(SERIAL_DATA, (uint8_t)c);
}

// ============================================================
// Impressão de strings
// ============================================================

void serial_print(const char *str) {
    if (!initialized) serial_init();
    while (*str) {
        serial_putc(*str++);
    }
}

// ============================================================
// Impressão de números (decimais)
// ============================================================

static void print_unsigned_dec(unsigned int num) {
    char buf[12];
    int i = 0;
    if (num == 0) {
        serial_putc('0');
        return;
    }
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) {
        serial_putc(buf[--i]);
    }
}

void serial_print_int(int num) {
    if (num < 0) {
        serial_putc('-');
        if (num == -2147483648) { // INT_MIN
            print_unsigned_dec(2147483648U);
            return;
        }
        print_unsigned_dec(-num);
    } else {
        print_unsigned_dec(num);
    }
}

void serial_print_uint(unsigned int num) {
    print_unsigned_dec(num);
}

// ============================================================
// Impressão de números (hexadecimal)
// ============================================================

static char hex_digit(uint8_t d) {
    return (d < 10) ? ('0' + d) : ('A' + (d - 10));
}

void serial_print_hex_raw(unsigned int num) {
    // 8 dígitos para 32 bits
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t d = (num >> i) & 0xF;
        serial_putc(hex_digit(d));
    }
}

void serial_print_hex(unsigned int num) {
    serial_print("0x");
    serial_print_hex_raw(num);
}

// ============================================================
// Serial printf simplificado
// ============================================================

void serial_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            serial_putc(*fmt++);
            continue;
        }

        fmt++; // pular '%'
        switch (*fmt) {
            case 'd': {
                int val = va_arg(args, int);
                serial_print_int(val);
                fmt++;
                break;
            }
            case 'u': {
                unsigned int val = va_arg(args, unsigned int);
                serial_print_uint(val);
                fmt++;
                break;
            }
            case 'x':
            case 'X': {
                unsigned int val = va_arg(args, unsigned int);
                serial_print_hex(val);
                fmt++;
                break;
            }
            case 's': {
                const char *str = va_arg(args, const char*);
                serial_print(str);
                fmt++;
                break;
            }
            case 'c': {
                char ch = (char)va_arg(args, int);
                serial_putc(ch);
                fmt++;
                break;
            }
            case '%': {
                serial_putc('%');
                fmt++;
                break;
            }
            default: {
                // caractere desconhecido – imprime literalmente
                serial_putc('%');
                serial_putc(*fmt++);
                break;
            }
        }
    }
    va_end(args);
}

// ============================================================
// Função de debug com tag
// ============================================================

void serial_debug(const char *tag, const char *msg) {
    serial_print("[");
    serial_print(tag);
    serial_print("] ");
    serial_print(msg);
    serial_print("\n");
}