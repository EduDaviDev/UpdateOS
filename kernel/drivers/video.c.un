#include "video.h"
#include "serial.h"
#include "../mboot.h"
#include "../cpu/paging.h"

static uint8_t *fb_addr   = 0;
static uint32_t fb_pitch  = 0;
static uint32_t fb_width  = 0;
static uint32_t fb_height = 0;
static uint8_t  fb_bytes_per_pixel = 0;

/* ------------------------------------------------------------------ */
int gfx_init(void) {
    if (!g_multiboot_info) {
        serial_print("[gfx] g_multiboot_info nulo\n");
        return 0;
    }

    uint8_t *p   = (uint8_t *)g_multiboot_info->tags;
    uint8_t *end = (uint8_t *)g_multiboot_info + g_multiboot_info->total_size;

    while (p < end) {
        struct multiboot_tag *tag = (struct multiboot_tag *)p;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) break;

        if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            struct multiboot_tag_framebuffer *fb =
                (struct multiboot_tag_framebuffer *)p;

            fb_addr   = (uint8_t *)(uintptr_t)fb->framebuffer_addr;
            fb_pitch  = fb->framebuffer_pitch;
            fb_width  = fb->framebuffer_width;
            fb_height = fb->framebuffer_height;
            fb_bytes_per_pixel = fb->framebuffer_bpp / 8;

            serial_print("[gfx] framebuffer: ");
            serial_print_dec(fb_width);  serial_print("x");
            serial_print_dec(fb_height); serial_print(" @ ");
            serial_print_dec(fb->framebuffer_bpp); serial_print("bpp");
            serial_print("  addr=");  serial_print_hex32((uint32_t)(uintptr_t)fb_addr);
            serial_print("  pitch="); serial_print_dec(fb_pitch);
            serial_print("  type=");  serial_print_dec(fb->framebuffer_type);
            serial_print("\n");

            /* Mapeia cada página física do framebuffer (identity mapping) */
            uintptr_t fb_phys = (uintptr_t)fb_addr;
            uint32_t  size    = fb_pitch * fb_height;
            uint32_t  pages   = (size + 0xFFF) / 0x1000;

            for (uint32_t i = 0; i < pages; i++) {
                paging_map_page((uint32_t)(fb_phys + i * 0x1000),
                                (uint32_t)(fb_phys + i * 0x1000),
                                0x3 /* present | rw */);
            }
            serial_print("[gfx] framebuffer mapeado: ");
            serial_print_dec(pages);
            serial_print(" paginas\n");
            return 1;
        }
        p += (tag->size + 7) & ~7u;
    }

    serial_print("[gfx] nenhuma tag FRAMEBUFFER no Multiboot2\n");
    return 0;
}

/* ------------------------------------------------------------------ */
void gfx_putpixel(uint32_t x, uint32_t y, uint32_t col) {
    if (!fb_addr) return;
    if (x >= fb_width || y >= fb_height) return;

    uint8_t *pixel = fb_addr + y * fb_pitch + x * fb_bytes_per_pixel;

    switch (fb_bytes_per_pixel) {
        case 4:
            *(volatile uint32_t *)pixel = col;
            break;
        case 3:
            pixel[0] = (uint8_t)( col        & 0xFF);
            pixel[1] = (uint8_t)((col >>  8) & 0xFF);
            pixel[2] = (uint8_t)((col >> 16) & 0xFF);
            break;
        case 2:
            *(volatile uint16_t *)pixel = (uint16_t)col;
            break;
        case 1:
            pixel[0] = (uint8_t)col;
            break;
    }
}

/* ------------------------------------------------------------------ */
void gfx_clear(uint32_t col) {
    if (!fb_addr) return;

    for (uint32_t y = 0; y < fb_height; y++) {
        uint8_t *row = fb_addr + y * fb_pitch;

        switch (fb_bytes_per_pixel) {
            case 4: {
                volatile uint32_t *p = (volatile uint32_t *)row;
                for (uint32_t x = 0; x < fb_width; x++) p[x] = col;
                break;
            }
            case 3:
                for (uint32_t x = 0; x < fb_width; x++) {
                    row[x*3 + 0] = (uint8_t)( col        & 0xFF);
                    row[x*3 + 1] = (uint8_t)((col >>  8) & 0xFF);
                    row[x*3 + 2] = (uint8_t)((col >> 16) & 0xFF);
                }
                break;
            case 2: {
                volatile uint16_t *p = (volatile uint16_t *)row;
                for (uint32_t x = 0; x < fb_width; x++) p[x] = (uint16_t)col;
                break;
            }
            case 1:
                for (uint32_t x = 0; x < fb_width; x++) row[x] = (uint8_t)col;
                break;
        }
    }
}