#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

/* Inicializa o driver a partir da tag FRAMEBUFFER do Multiboot2.
 * Mapeia o framebuffer no espaço virtual do kernel e aloca o
 * backbuffer em RAM.
 * Retorna 1 em sucesso, 0 caso contrário. */
int gfx_init(void);

/* --- Framebuffer real ---------------------------------------------- */

extern uint8_t  *gfx_fb_addr;      /* ponteiro pro framebuffer */
extern uint32_t  gfx_pitch;        /* bytes por linha */
extern uint32_t  gfx_width;        /* largura em pixels */
extern uint32_t  gfx_height;       /* altura em pixels */
extern uint32_t  gfx_bpp;          /* bits por pixel (8/16/24/32) */

/* --- Backbuffer 32bpp (0x00RRGGBB) --------------------------------- */

extern uint32_t *gfx_bb_addr;      /* buffer: [y*gfx_width + x] */
extern uint32_t  gfx_bb_pixels;    /* total de pixels */
extern uint32_t  gfx_bb_bytes;     /* total de bytes */

#endif /* VIDEO_H */