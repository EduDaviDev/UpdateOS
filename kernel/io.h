#ifndef KERNEL_IO_H
#define KERNEL_IO_H

#include <stdint.h>

/* ============================================================
 *  OUT — envia dados para uma porta de I/O
 * ============================================================ */

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

/* ============================================================
 *  IN — lê dados de uma porta de I/O
 * ============================================================ */

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* ============================================================
 *  I/O WAIT — pequeno atraso para dar tempo ao PIC/barramento
 *  Escrever em uma porta não usada (0x80) consome ~1 ciclo.
 * ============================================================ */

static inline void io_wait(void) {
    outb(0x80, 0);
}

/* ============================================================
 *  Alternativa portátil de io_wait (caso prefira)
 * ============================================================ */

static inline void io_wait_alt(void) {
    __asm__ volatile ("jmp 1f\n1:");
}

#endif /* KERNEL_IO_H */