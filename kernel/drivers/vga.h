#ifndef DRIVERS_VGA_H
#define DRIVERS_VGA_H

#include <stdint.h>

/* Dimensões do modo texto padrão do VGA */
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/* Cores (atributo = bg << 4 | fg) */
typedef enum
{
	VGA_BLACK = 0,
	VGA_BLUE,
	VGA_GREEN,
	VGA_CYAN,
	VGA_RED,
	VGA_MAGENTA,
	VGA_BROWN,
	VGA_LIGHT_GREY,
	VGA_DARK_GREY,
	VGA_LIGHT_BLUE,
	VGA_LIGHT_GREEN,
	VGA_LIGHT_CYAN,
	VGA_LIGHT_RED,
	VGA_LIGHT_MAGENTA,
	VGA_YELLOW,
	VGA_WHITE
} vga_color_t;

/* Posição atual do cursor na tela (globais) */
extern int vga_pos_x;
extern int vga_pos_y;

/* Cor padrão usada por vga_putcharhar / vga_print */
extern uint8_t vga_default_attr;

/* Inicialização */
void vga_init(void);
void vga_clear(void);

/* Saída de caracteres */
void vga_putchar(char c);
void vga_print(const char *s);
void vga_print_color(const char *s, vga_color_t fg, vga_color_t bg);
void vga_backspace(void);

/* Utilidades */
void vga_set_color(vga_color_t fg, vga_color_t bg);
void vga_set_cursor(int x, int y);
void vga_update_cursor(void);

#endif /* DRIVERS_VGA_H */