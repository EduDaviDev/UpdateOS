#include "mboot.h"
#include <stddef.h>

/* vga_printf opcional: se não existir, os dumps são no-op. */
extern int vga_printf(const char *fmt, ...) __attribute__((weak));

/* =====================================================================
 * Helpers internos
 * ===================================================================== */
static inline uint32_t align_up8(uint32_t x) { return (x + 7u) & ~7u; }

static int str_contains(const char *hay, const char *needle) {
    if (!*needle) return 1;
    for (; *hay; hay++) {
        const char *h = hay, *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return 1;
    }
    return 0;
}

/* =====================================================================
 * Verificação
 * ===================================================================== */
int mboot_verify_magic(uint32_t magic) {
    return magic == MULTIBOOT2_BOOTLOADER_MAGIC;
}

int mboot_is_valid(const struct multiboot_info *mbi) {
    if (!mbi) return 0;
    if (mbi->total_size < sizeof(struct multiboot_info)) return 0;
    if ((mbi->total_size & 7u) != 0) return 0;
    return 1;
}

/* =====================================================================
 * Iteração genérica
 * ===================================================================== */
struct multiboot_tag *mboot_find_tag_n(const struct multiboot_info *mbi,
                                       uint32_t type, uint32_t n) {
    if (!mbi) return NULL;

    uint8_t *p   = (uint8_t *)mbi->tags;
    uint8_t *end = (uint8_t *)mbi + mbi->total_size;
    uint32_t count = 0;

    while (p + sizeof(struct multiboot_tag) <= end) {
        struct multiboot_tag *tag = (struct multiboot_tag *)p;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) break;
        if (tag->size < 8) break;   /* proteção contra loop infinito */

        if (tag->type == type) {
            if (count == n) return tag;
            count++;
        }
        p += align_up8(tag->size);
    }
    return NULL;
}

struct multiboot_tag *mboot_find_tag(const struct multiboot_info *mbi, uint32_t type) {
    return mboot_find_tag_n(mbi, type, 0);
}

int mboot_has_tag(const struct multiboot_info *mbi, uint32_t type) {
    return mboot_find_tag(mbi, type) != NULL;
}

uint32_t mboot_count_tags(const struct multiboot_info *mbi) {
    if (!mbi) return 0;

    uint8_t *p   = (uint8_t *)mbi->tags;
    uint8_t *end = (uint8_t *)mbi + mbi->total_size;
    uint32_t count = 0;

    while (p + sizeof(struct multiboot_tag) <= end) {
        struct multiboot_tag *tag = (struct multiboot_tag *)p;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) break;
        if (tag->size < 8) break;
        count++;
        p += align_up8(tag->size);
    }
    return count;
}

uint32_t mboot_count_tags_of_type(const struct multiboot_info *mbi, uint32_t type) {
    if (!mbi) return 0;

    uint8_t *p   = (uint8_t *)mbi->tags;
    uint8_t *end = (uint8_t *)mbi + mbi->total_size;
    uint32_t count = 0;

    while (p + sizeof(struct multiboot_tag) <= end) {
        struct multiboot_tag *tag = (struct multiboot_tag *)p;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) break;
        if (tag->size < 8) break;
        if (tag->type == type) count++;
        p += align_up8(tag->size);
    }
    return count;
}

const char *mboot_tag_name(uint32_t type) {
    switch (type) {
    case MULTIBOOT_TAG_TYPE_END:              return "END";
    case MULTIBOOT_TAG_TYPE_CMDLINE:          return "CMDLINE";
    case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME: return "BOOT_LOADER_NAME";
    case MULTIBOOT_TAG_TYPE_MODULE:           return "MODULE";
    case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:    return "BASIC_MEMINFO";
    case MULTIBOOT_TAG_TYPE_BOOTDEV:          return "BOOTDEV";
    case MULTIBOOT_TAG_TYPE_MMAP:             return "MMAP";
    case MULTIBOOT_TAG_TYPE_VBE:              return "VBE";
    case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:      return "FRAMEBUFFER";
    case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:     return "ELF_SECTIONS";
    case MULTIBOOT_TAG_TYPE_APM:              return "APM";
    case MULTIBOOT_TAG_TYPE_EFI32:            return "EFI32";
    case MULTIBOOT_TAG_TYPE_EFI64:            return "EFI64";
    case MULTIBOOT_TAG_TYPE_SMBIOS:           return "SMBIOS";
    case MULTIBOOT_TAG_TYPE_ACPI_OLD:         return "ACPI_OLD";
    case MULTIBOOT_TAG_TYPE_ACPI_NEW:         return "ACPI_NEW";
    case MULTIBOOT_TAG_TYPE_NETWORK:          return "NETWORK";
    case MULTIBOOT_TAG_TYPE_EFI_MMAP:         return "EFI_MMAP";
    case MULTIBOOT_TAG_TYPE_EFI_BS:           return "EFI_BS";
    case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR:   return "LOAD_BASE_ADDR";
    default:                                  return "?";
    }
}

/* =====================================================================
 * Getters diretos
 * ===================================================================== */
const char *mboot_get_cmdline_tag(const struct multiboot_info *mbi) {
    struct multiboot_tag *t = mboot_find_tag(mbi, MULTIBOOT_TAG_TYPE_CMDLINE);
    return t ? ((struct multiboot_tag_string *)t)->string : NULL;
}

const char *mboot_get_bootloader_name_tag(const struct multiboot_info *mbi) {
    struct multiboot_tag *t = mboot_find_tag(mbi, MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME);
    return t ? ((struct multiboot_tag_string *)t)->string : NULL;
}

struct multiboot_tag_basic_meminfo *
mboot_get_basic_meminfo_tag(const struct multiboot_info *mbi) {
    return (struct multiboot_tag_basic_meminfo *)
           mboot_find_tag(mbi, MULTIBOOT_TAG_TYPE_BASIC_MEMINFO);
}

struct multiboot_tag_bootdev *mboot_get_bootdev_tag(const struct multiboot_info *mbi) {
    return (struct multiboot_tag_bootdev *)
           mboot_find_tag(mbi, MULTIBOOT_TAG_TYPE_BOOTDEV);
}

struct multiboot_tag_mmap *mboot_get_mmap_tag(const struct multiboot_info *mbi) {
    return (struct multiboot_tag_mmap *)
           mboot_find_tag(mbi, MULTIBOOT_TAG_TYPE_MMAP);
}

struct multiboot_tag_framebuffer_common *
mboot_get_framebuffer_tag(const struct multiboot_info *mbi) {
    return (struct multiboot_tag_framebuffer_common *)
           mboot_find_tag(mbi, MULTIBOOT_TAG_TYPE_FRAMEBUFFER);
}

struct multiboot_tag_elf_sections *
mboot_get_elf_sections_tag(const struct multiboot_info *mbi) {
    return (struct multiboot_tag_elf_sections *)
           mboot_find_tag(mbi, MULTIBOOT_TAG_TYPE_ELF_SECTIONS);
}

struct multiboot_tag_new_acpi *mboot_get_acpi_new_tag(const struct multiboot_info *mbi) {
    return (struct multiboot_tag_new_acpi *)
           mboot_find_tag(mbi, MULTIBOOT_TAG_TYPE_ACPI_NEW);
}

struct multiboot_tag_old_acpi *mboot_get_acpi_old_tag(const struct multiboot_info *mbi) {
    return (struct multiboot_tag_old_acpi *)
           mboot_find_tag(mbi, MULTIBOOT_TAG_TYPE_ACPI_OLD);
}

struct multiboot_tag_load_base_addr *
mboot_get_load_base_addr_tag(const struct multiboot_info *mbi) {
    return (struct multiboot_tag_load_base_addr *)
           mboot_find_tag(mbi, MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR);
}

const void *mboot_get_rsdp(const struct multiboot_info *mbi) {
    struct multiboot_tag_new_acpi *n = mboot_get_acpi_new_tag(mbi);
    if (n) return n->rsdp;

    struct multiboot_tag_old_acpi *o = mboot_get_acpi_old_tag(mbi);
    if (o) return o->rsdp;

    return NULL;
}

/* =====================================================================
 * Módulos
 * ===================================================================== */
uint32_t mboot_get_module_count(const struct multiboot_info *mbi) {
    return mboot_count_tags_of_type(mbi, MULTIBOOT_TAG_TYPE_MODULE);
}

struct multiboot_tag_module *
mboot_get_module_by_number(const struct multiboot_info *mbi, uint32_t n) {
    return (struct multiboot_tag_module *)
           mboot_find_tag_n(mbi, MULTIBOOT_TAG_TYPE_MODULE, n);
}

int mboot_read_module(const struct multiboot_tag_module *tag, struct mboot_module *out) {
    if (!tag || !out) return -1;
    if (tag->type != MULTIBOOT_TAG_TYPE_MODULE) return -1;
    if (tag->mod_end < tag->mod_start) return -1;

    out->start   = tag->mod_start;
    out->end     = tag->mod_end;
    out->size    = tag->mod_end - tag->mod_start;
    out->cmdline = tag->cmdline;      /* pode ser "" mas nunca NULL */
    out->tag     = tag;
    return 0;
}

int mboot_read_module_by_number(const struct multiboot_info *mbi, uint32_t n,
                                struct mboot_module *out) {
    struct multiboot_tag_module *tag = mboot_get_module_by_number(mbi, n);
    if (!tag) return -1;
    return mboot_read_module(tag, out);
}

int mboot_find_module_by_cmdline(const struct multiboot_info *mbi, const char *needle) {
    uint32_t count = mboot_get_module_count(mbi);
    for (uint32_t i = 0; i < count; i++) {
        struct multiboot_tag_module *tag = mboot_get_module_by_number(mbi, i);
        if (tag && str_contains(tag->cmdline, needle)) return (int)i;
    }
    return -1;
}

/* =====================================================================
 * Memory map
 * ===================================================================== */
uint32_t mboot_mmap_count(const struct multiboot_tag_mmap *mmap) {
    if (!mmap) return 0;
    if (mmap->entry_size == 0) return 0;
    uint32_t body = mmap->size - sizeof(struct multiboot_tag_mmap);
    return body / mmap->entry_size;
}

struct multiboot_mmap_entry *mboot_mmap_get(const struct multiboot_tag_mmap *mmap, uint32_t n) {
    if (!mmap || n >= mboot_mmap_count(mmap)) return NULL;
    return (struct multiboot_mmap_entry *)
           ((uint8_t *)mmap->entries + n * mmap->entry_size);
}

int mboot_mmap_is_available(const struct multiboot_tag_mmap *mmap,
                            uint64_t addr, uint64_t len) {
    if (!mmap || len == 0) return 0;
    uint64_t end = addr + len;
    uint32_t count = mboot_mmap_count(mmap);

    for (uint32_t i = 0; i < count; i++) {
        struct multiboot_mmap_entry *e = mboot_mmap_get(mmap, i);
        if (e->type != MULTIBOOT_MEMORY_AVAILABLE) continue;
        uint64_t e_end = e->addr + e->len;
        if (addr >= e->addr && end <= e_end) return 1;
    }
    return 0;
}

/* =====================================================================
 * Resumos de memória
 * ===================================================================== */
uint32_t mboot_get_mem_size_basic(const struct multiboot_info *mbi) {
    struct multiboot_tag_basic_meminfo *bm = mboot_get_basic_meminfo_tag(mbi);
    if (!bm) return 0;
    return (1024u * 1024u) + (bm->mem_upper * 1024u);
}

uint32_t mboot_get_mem_size_mmap(const struct multiboot_info *mbi) {
    struct multiboot_tag_mmap *mmap = mboot_get_mmap_tag(mbi);
    if (!mmap) return 0;

    uint32_t top = 0;
    uint32_t count = mboot_mmap_count(mmap);

    for (uint32_t i = 0; i < count; i++) {
        struct multiboot_mmap_entry *e = mboot_mmap_get(mmap, i);
        if (e->type != MULTIBOOT_MEMORY_AVAILABLE) continue;
        uint64_t hi = e->addr + e->len;
        if (hi > 0xFFFFFFFFull) hi = 0xFFFFFFFFull;
        if ((uint32_t)hi > top) top = (uint32_t)hi;
    }
    return top;
}

uint32_t mboot_get_mem_size(const struct multiboot_info *mbi) {
    uint32_t m = mboot_get_mem_size_mmap(mbi);
    if (m) return m;
    return mboot_get_mem_size_basic(mbi);
}

uint64_t mboot_get_total_available(const struct multiboot_info *mbi) {
    struct multiboot_tag_mmap *mmap = mboot_get_mmap_tag(mbi);
    if (!mmap) return 0;

    uint64_t total = 0;
    uint32_t count = mboot_mmap_count(mmap);

    for (uint32_t i = 0; i < count; i++) {
        struct multiboot_mmap_entry *e = mboot_mmap_get(mmap, i);
        if (e->type == MULTIBOOT_MEMORY_AVAILABLE) total += e->len;
    }
    return total;
}

/* =====================================================================
 * Framebuffer
 * ===================================================================== */
uint32_t mboot_fb_bytes_per_pixel(const struct multiboot_tag_framebuffer_common *fb) {
    if (!fb) return 0;
    return (fb->framebuffer_bpp + 7u) / 8u;
}

uint32_t mboot_fb_size_bytes(const struct multiboot_tag_framebuffer_common *fb) {
    if (!fb) return 0;
    return fb->framebuffer_pitch * fb->framebuffer_height;
}

int mboot_fb_is_rgb(const struct multiboot_tag_framebuffer_common *fb) {
    return fb && fb->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB;
}

int mboot_fb_is_text(const struct multiboot_tag_framebuffer_common *fb) {
    return fb && fb->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT;
}

/* =====================================================================
 * Dumps
 * ===================================================================== */
void mboot_mmap_dump(const struct multiboot_tag_mmap *mmap) {
    if (!vga_printf || !mmap) return;

    uint32_t count = mboot_mmap_count(mmap);
    vga_printf("[mmap] %u entradas (entry_size=%u ver=%u)\n",
               count, mmap->entry_size, mmap->entry_version);

    for (uint32_t i = 0; i < count; i++) {
        struct multiboot_mmap_entry *e = mboot_mmap_get(mmap, i);
        const char *t =
            (e->type == MULTIBOOT_MEMORY_AVAILABLE)        ? "avail"   :
            (e->type == MULTIBOOT_MEMORY_RESERVED)         ? "reserv"  :
            (e->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) ? "acpi"    :
            (e->type == MULTIBOOT_MEMORY_NVS)              ? "nvs"     :
            (e->type == MULTIBOOT_MEMORY_BADRAM)           ? "badram"  : "?";
        vga_printf("  [%u] 0x%08x%08x - 0x%08x%08x  %s\n",
                   i,
                   (uint32_t)(e->addr >> 32), (uint32_t)(e->addr & 0xFFFFFFFFu),
                   (uint32_t)((e->addr + e->len) >> 32),
                   (uint32_t)((e->addr + e->len) & 0xFFFFFFFFu),
                   t);
    }
}

void mboot_dump(const struct multiboot_info *mbi) {
    if (!vga_printf) return;
    if (!mbi) { vga_printf("[mboot] mbi NULL\n"); return; }

    vga_printf("[mboot] total_size=%u  tags=%u\n",
               mbi->total_size, mboot_count_tags(mbi));

    MBOOT_FOR_EACH_TAG(mbi, tag) {
        vga_printf("  tag type=%u (%s) size=%u\n",
                   tag->type, mboot_tag_name(tag->type), tag->size);
    }

    const char *bl = mboot_get_bootloader_name_tag(mbi);
    if (bl) vga_printf("  bootloader: %s\n", bl);

    const char *cl = mboot_get_cmdline_tag(mbi);
    if (cl) vga_printf("  cmdline   : \"%s\"\n", cl);

    struct multiboot_tag_basic_meminfo *bm = mboot_get_basic_meminfo_tag(mbi);
    if (bm) vga_printf("  mem_lower=%u KiB  mem_upper=%u KiB\n",
                       bm->mem_lower, bm->mem_upper);

    vga_printf("  modules   : %u\n", mboot_get_module_count(mbi));
    vga_printf("  mem_size  : %u MiB\n", mboot_get_mem_size(mbi) / (1024u * 1024u));
}

/* =====================================================================
 * One-liners de memory map
 * ===================================================================== */
uint64_t mboot_mmap_highest_available(const struct multiboot_tag_mmap *mmap) {
    if (!mmap) return 0;
    uint64_t top = 0;
    MBOOT_FOR_EACH_MMAP_ENTRY(mmap, e) {
        if (e->type != MULTIBOOT_MEMORY_AVAILABLE) continue;
        uint64_t hi = e->addr + e->len;
        if (hi > top) top = hi;
    }
    return top;
}

uint64_t mboot_mmap_highest_any(const struct multiboot_tag_mmap *mmap) {
    if (!mmap) return 0;
    uint64_t top = 0;
    MBOOT_FOR_EACH_MMAP_ENTRY(mmap, e) {
        uint64_t hi = e->addr + e->len;
        if (hi > top) top = hi;
    }
    return top;
}

uint64_t mboot_mmap_first_fit(const struct multiboot_tag_mmap *mmap,
                              uint64_t min_addr, uint64_t size) {
    if (!mmap || size == 0) return 0;
    MBOOT_FOR_EACH_MMAP_ENTRY(mmap, e) {
        if (e->type != MULTIBOOT_MEMORY_AVAILABLE) continue;
        if (e->len < size) continue;
        uint64_t start = e->addr;
        if (start < min_addr) start = min_addr;
        if (start + size <= e->addr + e->len) return start;
    }
    return 0;
}

struct multiboot_mmap_entry *mboot_mmap_find(const struct multiboot_tag_mmap *mmap,
                                             uint64_t addr) {
    if (!mmap) return NULL;
    MBOOT_FOR_EACH_MMAP_ENTRY(mmap, e) {
        if (addr >= e->addr && addr < e->addr + e->len) return e;
    }
    return NULL;
}

struct multiboot_mmap_entry *mboot_mmap_find_available(const struct multiboot_tag_mmap *mmap,
                                                       uint64_t addr) {
    struct multiboot_mmap_entry *e = mboot_mmap_find(mmap, addr);
    return (e && e->type == MULTIBOOT_MEMORY_AVAILABLE) ? e : NULL;
}

/* =====================================================================
 * Resumo de uma linha
 * ===================================================================== */
void mboot_summary(const struct multiboot_info *mbi) {
    if (!vga_printf || !mbi) return;

    const char *bl = mboot_bootloader(mbi);
    struct multiboot_tag_mmap *mmap = mboot_get_mmap_tag(mbi);
    uint32_t mods = mboot_get_module_count(mbi);

    vga_printf("[mboot] %s | %u MiB | %u mod | mmap=%s\n",
               bl ? bl : "?",
               mboot_mem_mib(mbi),
               mods,
               mmap ? "sim" : "nao");
}