#pragma once

#include <stdint.h>
#include "drivers/video.h"

/* ============================================================
 *  updgr — biblioteca gráfica do UpdateOS
 *  Todas as coordenadas são em pixels. (0,0) = canto superior esq.
 *  Todas as cores são 0x00RRGGBB.
 * ============================================================ */

/* ============================ Info ============================ */

uint32_t gfx_get_width(void);
uint32_t gfx_get_height(void);
uint32_t gfx_get_bpp(void);

/* ============================ Cores ============================ */

static inline uint32_t gfx_rgb(uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}
static inline uint32_t gfx_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}
static inline uint8_t gfx_r(uint32_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t gfx_g(uint32_t c) { return (c >>  8) & 0xFF; }
static inline uint8_t gfx_b(uint32_t c) { return  c        & 0xFF; }
static inline uint8_t gfx_a(uint32_t c) { return (c >> 24) & 0xFF; }

/* Blend alpha (0-255) entre fg e bg. */
uint32_t gfx_blend(uint32_t fg, uint32_t bg, uint8_t alpha);

/* ============================ Clipping ============================ */

void gfx_set_clip(int x, int y, int w, int h);
void gfx_reset_clip(void);
void gfx_get_clip(int *x, int *y, int *w, int *h);

/* ============================ Básico ============================ */

void     gfx_clear(uint32_t color);
void     gfx_putpixel(int x, int y, uint32_t color);
uint32_t gfx_getpixel(int x, int y);

/* Copia o backbuffer pro framebuffer real (apresenta o frame). */
void     gfx_refresh(void);

/* ============================ Linhas ============================ */

void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void gfx_draw_hline(int x, int y, int w, uint32_t color);
void gfx_draw_vline(int x, int y, int h, uint32_t color);

/* ============================ Retângulos ============================ */

void gfx_draw_rect(int x, int y, int width, int height, uint32_t color);
void gfx_fill_rect(int x, int y, int width, int height, uint32_t color);

/* Gradiente vertical: c0 no topo, c1 na base. */
void gfx_fill_rect_grad_v(int x, int y, int w, int h, uint32_t c0, uint32_t c1);
/* Gradiente horizontal: c0 à esquerda, c1 à direita. */
void gfx_fill_rect_grad_h(int x, int y, int w, int h, uint32_t c0, uint32_t c1);

/* ============================ Círculos ============================ */

void gfx_draw_circle(int x, int y, int radius, uint32_t color);
void gfx_fill_circle(int x, int y, int radius, uint32_t color);

/* ============================ Elipses ============================ */

void gfx_draw_ellipse(int x, int y, int width, int height, uint32_t color);
void gfx_fill_ellipse(int x, int y, int width, int height, uint32_t color);

/* ============================ Triângulos ============================ */

void gfx_draw_triangle(int x0, int y0, int x1, int y1,
                       int x2, int y2, uint32_t color);
void gfx_fill_triangle(int x0, int y0, int x1, int y1,
                       int x2, int y2, uint32_t color);

/* ============================ Polígonos ============================ */

/* positions = count vértices, sem terminador. */
void gfx_draw_polygon(const int positions[][2], int count, uint32_t color);
void gfx_fill_polygon(const int positions[][2], int count, uint32_t color);

/* ============================ Superfícies / Blit ============================ */

typedef struct {
	int       width;
	int       height;
	uint32_t *pixels;   /* [y * width + x], 0x00RRGGBB */
} gfx_surface_t;

/* Copia a superfície inteira para o backbuffer em (dx,dy). */
void gfx_blit(gfx_surface_t *src, int dx, int dy);

/* Como gfx_blit, mas ignora pixels iguais a key. */
void gfx_blit_colorkey(gfx_surface_t *src, int dx, int dy, uint32_t key);

/* Como gfx_blit, mas aplica alpha global (0-255). */
void gfx_blit_alpha(gfx_surface_t *src, int dx, int dy, uint8_t alpha);

/* ============================ Fontes ============================ */

#define GFX_FLAG_FLIP_H  0x01
#define GFX_FLAG_FLIP_V  0x02
#define GFX_FLAG_FLIP_HV 0x03

typedef struct
{
	char     name[32];
	char     author[32];
	int      width;
	int      height;
	uint8_t *code;
	int      charcount;
	int      first_char;
	int      last_char;
	uint8_t  flags;
} gfx_font_t;

typedef struct
{
	char        name[32];
	gfx_font_t *fonts;
	int         fontcount;
} gfx_fpack_t;

/* Desenha texto em (x,y). Suporta '\n' e '\r'. */
void gfx_print(int x, int y, const char *s, uint32_t color);

/* Desenha um único caractere. */
void gfx_print_char(int x, int y, char c, uint32_t color);

void gfx_setfont(gfx_font_t font);
void gfx_setfpack(gfx_fpack_t fpack, int font);

/* Largura/altura em pixels do texto com a fonte atual. */
int gfx_text_width(const char *s);
int gfx_text_height(void);

extern gfx_font_t current_font;