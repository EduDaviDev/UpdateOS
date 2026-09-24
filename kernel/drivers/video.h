#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

/* Inicializa o driver a partir da tag FRAMEBUFFER do Multiboot2.
 * Também mapeia o framebuffer no espaço virtual do kernel.
 * Retorna 1 em sucesso, 0 caso contrário. */
int gfx_init(void);

/* Escreve um pixel. col = 0x00RRGGBB (byte alto ignorado).
 * Fora dos limites da tela, é ignorado silenciosamente. */
void gfx_putpixel(uint32_t x, uint32_t y, uint32_t col);

/* Preenche a tela inteira com a cor. */
void gfx_clear(uint32_t col);

#endif /* VIDEO_H */