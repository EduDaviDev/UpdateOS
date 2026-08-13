#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stddef.h>
#include "serial.h"

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

/* --- Funções de sistema --- */

// Reinicializa o sistema (reset) via controlador de teclado (porta 0x64)
static inline void reboot(void) {
    // Tenta reset via controlador de teclado
    outb(0x64, 0xFE);
    // Fallback: se não funcionar, tenta escrever na porta 0xCF9 (via PCI)
    outb(0xCF9, 0x06);
    // Espera eterna (caso não reinicie)
    while (1) __asm__ volatile ("hlt");
}

// Desliga o sistema (shutdown) – compatível com QEMU e hardware moderno (ACPI)
static inline void shutdown(void) {
    // Tenta desligar via QEMU (porta 0x604 - valor 0x2000)
    outl(0x604, 0x2000);
    // Tenta via VMware (porta 0x8900 - valor 0x0000)
    outw(0x8900, 0x0000);
    // Tenta via ACPI (porta 0x4004 - valor 0x0000)
    outw(0x4004, 0x0000);
    // Tenta via porta 0x604 com valor 0x3000 (outro método)
    outl(0x604, 0x3000);
    // Se nada funcionar, entra em loop infinito
    while (1) __asm__ volatile ("hlt");
}

#endif /* IO_H */