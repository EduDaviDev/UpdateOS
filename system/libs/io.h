#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stddef.h>

/* --- Portas I/O básicas --- */
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t val;
    __asm__ volatile ("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t val;
    __asm__ volatile ("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

/* --- Serial (COM1) --- */
#define COM1_PORT 0x3F8

/* Verifica se a porta serial está pronta para enviar */
static inline int serial_is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

/* Envia um único caractere pela serial */
static inline void serial_putc(char c) {
    while (!serial_is_transmit_empty()) { /* espera */ }
    outb(COM1_PORT, c);
}

/* Envia uma string pela serial (para depuração) */
static inline void serial_print(const char *str) {
    while (*str) {
        serial_putc(*str++);
    }
}

/* Inicializa a porta serial (baud rate 115200, 8 bits, sem paridade, 1 stop bit) */
static inline void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);    // Desabilita interrupções
    outb(COM1_PORT + 3, 0x80);    // Habilita DLAB (para setar baud rate)
    outb(COM1_PORT, 0x01);        // Divisor = 1 (115200 baud)
    outb(COM1_PORT + 1, 0x00);    // High divisor (0)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, sem paridade, 1 stop bit
    outb(COM1_PORT + 2, 0xC7);    // FIFO enable, clear, 14 bytes threshold
    outb(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

#endif /* IO_H */