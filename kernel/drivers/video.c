#include "video.h"
#include "serial.h"
#include "../mboot.h"
#include "../cpu/paging.h"
#include "../libs/memory.h"
#include "../libs/string.h"

/* --- Estado global ------------------------------------------------- */

uint8_t  *gfx_fb_addr   = 0;
uint32_t  gfx_pitch     = 0;
uint32_t  gfx_width     = 0;
uint32_t  gfx_height    = 0;
uint32_t  gfx_bpp       = 0;

uint32_t *gfx_bb_addr   = 0;
uint32_t  gfx_bb_pixels = 0;
uint32_t  gfx_bb_bytes  = 0;

/* ==================================================================== */
/*  MSR helpers                                                          */
/* ==================================================================== */

static inline void wrmsr(uint32_t msr, uint32_t lo, uint32_t hi) {
    __asm__ volatile("wrmsr" :: "c"(msr), "a"(lo), "d"(hi));
}
static inline void rdmsr(uint32_t msr, uint32_t *lo, uint32_t *hi) {
    __asm__ volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

#define IA32_MTRRCAP   0x0FE
#define IA32_MTRRDEF   0x2FF
#define IA32_MTRRBASE  0x200
#define IA32_MTRRMASK  0x201

/* ==================================================================== */
/*  MTRR                                                                 */
/* ==================================================================== */

static int mtrr_compute_region(uint32_t base, uint32_t size,
                               uint32_t *out_base, uint32_t *out_region)
{
    if (size == 0) return 0;
    uint32_t region = 4096u;
    while (region < size) {
        if (region >= 0x80000000u) return 0;
        region <<= 1;
    }
    uint32_t aligned = base & ~(region - 1);
    if (aligned > 0xFFFFFFFFu - (region - 1)) return 0;
    *out_base   = aligned;
    *out_region = region;
    return 1;
}

static int mtrr_set_wc(uint32_t base, uint32_t size) {
    uint32_t region_base, region;
    if (!mtrr_compute_region(base, size, &region_base, &region)) {
        serial_print("[mtrr] regiao nao representavel\n");
        return 0;
    }

    uint32_t cap_lo, cap_hi;
    rdmsr(IA32_MTRRCAP, &cap_lo, &cap_hi);
    uint32_t var_count = cap_lo & 0xFF;
    if (var_count == 0) {
        serial_print("[mtrr] CPU sem MTRRs variaveis\n");
        return 0;
    }

    int slot = -1;
    for (uint32_t i = 0; i < var_count; i++) {
        uint32_t lo, hi;
        rdmsr(IA32_MTRRBASE + 2*i, &lo, &hi);
        if (!(lo & 0x800)) { slot = (int)i; break; }
    }
    if (slot < 0) {
        serial_print("[mtrr] sem slot livre\n");
        return 0;
    }

    uint32_t base_lo = (region_base & 0xFFFFF000u) | 0x01u;
    uint32_t mask_lo = (~(region - 1) & 0xFFFFF000u) | 0x800u;

    wrmsr(IA32_MTRRBASE + 2*slot, base_lo, 0);
    wrmsr(IA32_MTRRMASK + 2*slot, mask_lo, 0);

    uint32_t def_lo, def_hi;
    rdmsr(IA32_MTRRDEF, &def_lo, &def_hi);
    def_lo |= (1u << 11);
    wrmsr(IA32_MTRRDEF, def_lo, def_hi);

    serial_print("[mtrr] slot="); serial_print_dec((uint32_t)slot);
    serial_print(" base=");  serial_print_hex32(region_base);
    serial_print(" size=");  serial_print_dec(region);
    serial_print("\n");

	wrmsr(IA32_MTRRBASE + 2*slot, base_lo, 0);
	wrmsr(IA32_MTRRMASK + 2*slot, mask_lo, 0);
	rdmsr(IA32_MTRRDEF, &def_lo, &def_hi);
	def_lo |= (1u << 11);
	wrmsr(IA32_MTRRDEF, def_lo, def_hi);
	serial_print("[mtrr] slot="); serial_print_dec((uint32_t)slot);
	serial_print(" base=");  serial_print_hex32(region_base);
	serial_print(" size=");  serial_print_dec(region);
	serial_print("\n");
	/* ---- readback de confirmação ---- */
	uint32_t rb_lo, rb_hi, rm_lo, rm_hi;
	rdmsr(IA32_MTRRBASE + 2*slot, &rb_lo, &rb_hi);
	rdmsr(IA32_MTRRMASK + 2*slot, &rm_lo, &rm_hi);
	serial_print("[mtrr] readback base="); serial_print_hex32(rb_lo);
	serial_print(" mask=");               serial_print_hex32(rm_lo);
	serial_print("\n");
	uint32_t d_lo, d_hi;
	rdmsr(IA32_MTRRDEF, &d_lo, &d_hi);
	serial_print("[mtrr] def="); serial_print_hex32(d_lo);
	serial_print(" E=");         serial_print_dec((d_lo >> 11) & 1);
	serial_print("\n");
	uint32_t cr0;
	__asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
	serial_print("[mtrr] CR0="); serial_print_hex32(cr0);
	serial_print(" CD=");        serial_print_dec((cr0 >> 30) & 1);
	serial_print(" NW=");        serial_print_dec((cr0 >> 29) & 1);
	serial_print("\n");
	/* ---- fim readback ---- */

    return 1;
}

/* ==================================================================== */
/*  Init                                                                 */
/* ==================================================================== */

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

            gfx_fb_addr = (uint8_t *)(uintptr_t)fb->framebuffer_addr;
            gfx_pitch   = fb->framebuffer_pitch;
            gfx_width   = fb->framebuffer_width;
            gfx_height  = fb->framebuffer_height;
            gfx_bpp     = fb->framebuffer_bpp;

            serial_print("[gfx] framebuffer: ");
            serial_print_dec(gfx_width);  serial_print("x");
            serial_print_dec(gfx_height); serial_print(" @ ");
            serial_print_dec(gfx_bpp);    serial_print("bpp");
            serial_print("  addr=");  serial_print_hex32((uint32_t)(uintptr_t)gfx_fb_addr);
            serial_print("  pitch="); serial_print_dec(gfx_pitch);
            serial_print("\n");

            uintptr_t fb_phys = (uintptr_t)gfx_fb_addr;
            uint32_t  size    = gfx_pitch * gfx_height;
            uint32_t  pages   = (size + 0xFFF) / 0x1000;

            for (uint32_t i = 0; i < pages; i++) {
                paging_map_page((uint32_t)(fb_phys + i * 0x1000),
                                (uint32_t)(fb_phys + i * 0x1000),
                                0x3);
            }
            serial_print("[gfx] framebuffer mapeado: ");
            serial_print_dec(pages); serial_print(" paginas\n");

            if (!mtrr_set_wc((uint32_t)(uintptr_t)gfx_fb_addr, size)) {
                serial_print("[gfx] aviso: MTRR WC indisponivel\n");
            }

            /* Backbuffer 32bpp em RAM */
            gfx_bb_pixels = gfx_width * gfx_height;
            gfx_bb_bytes  = gfx_bb_pixels * sizeof(uint32_t);

            gfx_bb_addr = (uint32_t *)malloc(gfx_bb_bytes);
            if (!gfx_bb_addr) {
                serial_print("[gfx] FALHA backbuffer\n");
                return 0;
            }
            memset(gfx_bb_addr, 0, gfx_bb_bytes);

            serial_print("[gfx] backbuffer alocado: ");
            serial_print_dec(gfx_bb_bytes); serial_print(" bytes\n");
            return 1;
        }
        p += (tag->size + 7) & ~7u;
    }

    serial_print("[gfx] nenhuma tag FRAMEBUFFER\n");
    return 0;
}