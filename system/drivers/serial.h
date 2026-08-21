#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================
// Funções de saída serial (COM1)
// ============================================================

// Inicializa a porta serial (9600 baud, 8N1)
void serial_init(void);

// Envia um caractere (com polling)
void serial_putc(char c);

// Envia uma string
void serial_print(const char *str);

// Imprime um inteiro com sinal em decimal
void serial_print_int(int num);

// Imprime um inteiro sem sinal em decimal
void serial_print_uint(unsigned int num);

// Imprime um inteiro em hexadecimal com prefixo "0x" (8 dígitos)
void serial_print_hex(unsigned int num);

// Imprime um inteiro em hexadecimal sem prefixo
void serial_print_hex_raw(unsigned int num);

// Printf simplificado – suporta: %d, %u, %x, %X, %s, %c, %%
void serial_printf(const char *fmt, ...);

// Envia uma mensagem de debug com timestamp (opcional)
void serial_debug(const char *tag, const char *msg);

#endif // SERIAL_H