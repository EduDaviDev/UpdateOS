#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>

/* Constantes do modo texto VGA */
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000

/* Cores padrão (paleta CGA/VGA) */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_WHITE         = 15,
} vga_color_t;

/* Inicializa o driver VGA (limpa a tela, zera o cursor) */
void vga_init(void);

/* Limpa toda a tela com a cor de fundo padrão */
void vga_clear(void);

/* Escreve um único caractere na posição atual do cursor */
void vga_putc(char c);

/* Escreve uma string terminada em '\0' */
void vga_print(const char *str);

/* Escreve um número em decimal */
void vga_print_dec(uint32_t num);

/* Escreve um número em hexadecimal (sem prefixo 0x) */
void vga_print_hex(uint32_t num);

/* Define a cor do texto (primeiro plano e fundo) */
void vga_set_color(vga_color_t fg, vga_color_t bg);

#endif /* VGA_H */