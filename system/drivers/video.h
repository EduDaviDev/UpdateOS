#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ==============================
// Cores VGA (modo texto)
// ==============================
#define VGA_COL_BLACK         0
#define VGA_COL_BLUE          1
#define VGA_COL_GREEN         2
#define VGA_COL_CYAN          3
#define VGA_COL_RED           4
#define VGA_COL_MAGENTA       5
#define VGA_COL_BROWN         6
#define VGA_COL_LIGHT_GRAY    7
#define VGA_COL_DARK_GRAY     8
#define VGA_COL_LIGHT_BLUE    9
#define VGA_COL_LIGHT_GREEN   10
#define VGA_COL_LIGHT_CYAN    11
#define VGA_COL_LIGHT_RED     12
#define VGA_COL_LIGHT_MAGENTA 13
#define VGA_COL_YELLOW        14
#define VGA_COL_WHITE         15

// Macro para combinar background (nibble alto) e foreground (nibble baixo)
#define VGA_COLOR(bg, fg)  (((bg) << 4) | ((fg) & 0x0F))

// ==============================
// Configurações da tela VGA (modo texto 80x25)
// ==============================
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// ==============================
// Posição atual do cursor (acesso direto pelo usuário)
// ==============================
extern uint8_t txt_curX;
extern uint8_t txt_curY;

// ==============================
// Funções do driver de vídeo
// ==============================

// Inicializa o driver (limpa a tela com fundo preto, texto branco)
void video_init(void);

// Define a cor para as próximas impressões (bg e fg: 0-15)
void txt_setcol(uint8_t bg, uint8_t fg);

// Define a cor a partir de um atributo completo (byte)
void txt_setattr(uint8_t attr);

// Move o cursor sem imprimir (atualiza variáveis e hardware)
void txt_jump(uint8_t x, uint8_t y);

// Imprime uma string na posição atual do cursor (com rolagem)
void txt_print(const char *str);

// Imprime um caractere na posição atual
void txt_putc(char c);

// Imprime uma string em posição absoluta (não altera o cursor)
void txt_pos_print(uint8_t x, uint8_t y, const char *str);

// Coloca um caractere em posição absoluta
void txt_pos_putc(uint8_t x, uint8_t y, char c);

// Preenche um retângulo com um caractere (usando a cor atual)
void txt_fill(uint8_t x, uint8_t y, uint8_t dx, uint8_t dy, char c);

// Limpa a tela com espaços e a cor atual
void txt_clear(void);

// Limpa a tela com um caractere específico e a cor atual
void txt_clchar(char c);

// Mostra ou oculta o cursor (hardware VGA)
void vga_cur_show(bool show);

// ==============================
// Funções de impressão formatada
// ==============================

// Imprime um inteiro com sinal em decimal
void txt_print_int(int num);

// Imprime um inteiro sem sinal em hexadecimal (com prefixo "0x")
void txt_print_hex(unsigned int num);

// Imprime um inteiro sem sinal em decimal
void txt_print_uint(unsigned int num);

// Printf simplificado – suporta: %d, %u, %x, %X, %s, %c, %%
void txt_printf(const char *fmt, ...);

// Nova linha
void txt_newl(void);

#endif