#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stddef.h>

/* Porta COM1 (padrão) */
#define SERIAL_COM1 0x3F8

/* Inicializa COM1 em 38400 baud, 8N1.
 * Retorna 1 em sucesso, 0 em falha. */
int  serial_init(void);

/* Escrita/leitura de baixo nível */
void serial_putc(char c);
char serial_getc(void);           /* bloqueia até ter dado */
int  serial_received(void);       /* 1 se há byte disponível */
int  serial_is_transmit_empty(void);

/* Escrita de alto nível */
void serial_print(const char *s);
void serial_write(const char *s, size_t len);

/* Helpers de debug (formatam em decimal/hex sem libc) */
void serial_print_dec(uint32_t v);
void serial_print_hex32(uint32_t v);
void serial_print_hex64(uint64_t v);

/* Chamada pelo handler de #PF para reportar o estado do CPU */
void serial_dump_regs(uint32_t eip, uint32_t cr2, uint32_t err);

#endif /* SERIAL_H */