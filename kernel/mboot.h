#ifndef MBOOT_H
#define MBOOT_H

#include <stdint.h>

/* ====================================================================
 * Magic e tipos de tag
 * ==================================================================== */
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

#define MULTIBOOT_TAG_TYPE_END               0
#define MULTIBOOT_TAG_TYPE_CMDLINE           1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME  2
#define MULTIBOOT_TAG_TYPE_MODULE            3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO     4
#define MULTIBOOT_TAG_TYPE_BOOTDEV           5
#define MULTIBOOT_TAG_TYPE_MMAP              6
#define MULTIBOOT_TAG_TYPE_VBE               7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER       8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS      9
#define MULTIBOOT_TAG_TYPE_APM               10
#define MULTIBOOT_TAG_TYPE_EFI32             11
#define MULTIBOOT_TAG_TYPE_EFI64             12
#define MULTIBOOT_TAG_TYPE_SMBIOS            13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD          14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW          15
#define MULTIBOOT_TAG_TYPE_NETWORK           16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP          17
#define MULTIBOOT_TAG_TYPE_EFI_BS            18
#define MULTIBOOT_TAG_TYPE_EFI32_IH          19
#define MULTIBOOT_TAG_TYPE_EFI64_IH          20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR    21

/* Tipos de entrada do memory map */
#define MULTIBOOT_MEMORY_AVAILABLE        1
#define MULTIBOOT_MEMORY_RESERVED         2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS              4
#define MULTIBOOT_MEMORY_BADRAM           5

/* Tipos de framebuffer */
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED  0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB      1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT 2

/* ====================================================================
 * Estruturas de tag
 * ==================================================================== */
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

struct multiboot_tag_end {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

struct multiboot_tag_string {
    uint32_t type;
    uint32_t size;
    char     string[];
} __attribute__((packed));

struct multiboot_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char     cmdline[];
} __attribute__((packed));

struct multiboot_tag_basic_meminfo {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;
    uint32_t mem_upper;
} __attribute__((packed));

struct multiboot_tag_bootdev {
    uint32_t type;
    uint32_t size;
    uint32_t biosdev;
    uint32_t slice;
    uint32_t part;
} __attribute__((packed));

struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed));

struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot_mmap_entry entries[];
} __attribute__((packed));

struct multiboot_tag_vbe {
    uint32_t type;
    uint32_t size;
    /* campos específicos raramente usados — ignorados por ora */
} __attribute__((packed));

struct multiboot_tag_framebuffer_common {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint16_t reserved;
} __attribute__((packed));

struct multiboot_tag_framebuffer {
    struct multiboot_tag_framebuffer_common common;
    /* palette ou info RGB segue depois, conforme tipo */
} __attribute__((packed));

struct multiboot_tag_elf_sections {
    uint32_t type;
    uint32_t size;
    uint32_t num;
    uint32_t entsize;
    uint32_t shndx;
    char     sections[];
} __attribute__((packed));

struct multiboot_tag_new_acpi {
    uint32_t type;
    uint32_t size;
    uint8_t  rsdp[];
} __attribute__((packed));

struct multiboot_tag_old_acpi {
    uint32_t type;
    uint32_t size;
    uint8_t  rsdp[];
} __attribute__((packed));

struct multiboot_tag_load_base_addr {
    uint32_t type;
    uint32_t size;
    uint32_t load_base_addr;
} __attribute__((packed));

/* ====================================================================
 * Estrutura principal
 * ==================================================================== */
struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
    struct multiboot_tag tags[];
} __attribute__((packed));

/* ====================================================================
 * Estrutura de alto nível: módulo carregado
 * ==================================================================== */
struct mboot_module {
    uint32_t start;              /* endereço físico inicial */
    uint32_t end;                /* endereço físico final   */
    uint32_t size;               /* end - start             */
    const char *cmdline;         /* nunca NULL; pode ser "" */
    const struct multiboot_tag_module *tag;  /* tag original */
};

/* ====================================================================
 * Iteração limpa por tags
 *
 * Uso:
 *   MBOOT_FOR_EACH_TAG(mbi, tag) {
 *       // use 'tag' (struct multiboot_tag *)
 *   }
 * ==================================================================== */
#define MBOOT_FOR_EACH_TAG(mbi, tag)                                              \
    for (struct multiboot_tag *tag = (struct multiboot_tag *)(mbi)->tags,         \
         *_mb_end = (struct multiboot_tag *)((uint8_t *)(mbi) + (mbi)->total_size);\
         (uint8_t *)tag < (uint8_t *)_mb_end &&                                   \
         tag->type != MULTIBOOT_TAG_TYPE_END;                                     \
         tag = (struct multiboot_tag *)((uint8_t *)tag + ((tag->size + 7u) & ~7u)))

/* ====================================================================
 * API — Verificação
 * ==================================================================== */
int      mboot_verify_magic(uint32_t magic);
int      mboot_is_valid(const struct multiboot_info *mbi);

/* ====================================================================
 * API — Iteração genérica de tags
 * ==================================================================== */
struct multiboot_tag *mboot_find_tag  (const struct multiboot_info *mbi, uint32_t type);
struct multiboot_tag *mboot_find_tag_n(const struct multiboot_info *mbi, uint32_t type, uint32_t n);
int      mboot_has_tag             (const struct multiboot_info *mbi, uint32_t type);
uint32_t mboot_count_tags          (const struct multiboot_info *mbi);
uint32_t mboot_count_tags_of_type  (const struct multiboot_info *mbi, uint32_t type);
const char *mboot_tag_name         (uint32_t type);

/* ====================================================================
 * API — Getters diretos
 * ==================================================================== */
const char *mboot_get_cmdline_tag(const struct multiboot_info *mbi);
const char *mboot_get_bootloader_name_tag(const struct multiboot_info *mbi);

struct multiboot_tag_basic_meminfo *mboot_get_basic_meminfo_tag(const struct multiboot_info *mbi);
struct multiboot_tag_bootdev       *mboot_get_bootdev_tag(const struct multiboot_info *mbi);
struct multiboot_tag_mmap          *mboot_get_mmap_tag(const struct multiboot_info *mbi);
struct multiboot_tag_framebuffer_common *
                                    mboot_get_framebuffer_tag(const struct multiboot_info *mbi);
struct multiboot_tag_elf_sections  *mboot_get_elf_sections_tag(const struct multiboot_info *mbi);
struct multiboot_tag_new_acpi      *mboot_get_acpi_new_tag(const struct multiboot_info *mbi);
struct multiboot_tag_old_acpi      *mboot_get_acpi_old_tag(const struct multiboot_info *mbi);
struct multiboot_tag_load_base_addr*
                                    mboot_get_load_base_addr_tag(const struct multiboot_info *mbi);

/* RSDP do ACPI (prefere tag NEW; cai para OLD). */
const void *mboot_get_rsdp(const struct multiboot_info *mbi);

/* ====================================================================
 * API — Módulos
 * ==================================================================== */
uint32_t mboot_get_module_count(const struct multiboot_info *mbi);
struct multiboot_tag_module *mboot_get_module_by_number(const struct multiboot_info *mbi, uint32_t n);

/* Preenche 'out' a partir da tag. Devolve 0 em sucesso, -1 em erro. */
int mboot_read_module(const struct multiboot_tag_module *tag, struct mboot_module *out);

/* Conveniência: acha a n-ésima tag e preenche 'out'. */
int mboot_read_module_by_number(const struct multiboot_info *mbi, uint32_t n,
                                struct mboot_module *out);

/* Devolve índice do primeiro módulo cujo cmdline contém 'needle', ou -1. */
int mboot_find_module_by_cmdline(const struct multiboot_info *mbi, const char *needle);

/* ====================================================================
 * API — Memory map
 * ==================================================================== */
uint32_t mboot_mmap_count(const struct multiboot_tag_mmap *mmap);
struct multiboot_mmap_entry *mboot_mmap_get(const struct multiboot_tag_mmap *mmap, uint32_t n);

/* Devolve 1 se [addr, addr+len) está inteiramente em uma região AVAILABLE. */
int mboot_mmap_is_available(const struct multiboot_tag_mmap *mmap,
                            uint64_t addr, uint64_t len);

/* ====================================================================
 * API — Memória (resumos)
 * ==================================================================== */
uint32_t mboot_get_mem_size         (const struct multiboot_info *mbi);  /* p/ identity map */
uint32_t mboot_get_mem_size_basic   (const struct multiboot_info *mbi);
uint32_t mboot_get_mem_size_mmap    (const struct multiboot_info *mbi);
uint64_t mboot_get_total_available  (const struct multiboot_info *mbi);  /* soma de AVAILABLE */

/* ====================================================================
 * API — Framebuffer (helpers)
 * ==================================================================== */
uint32_t mboot_fb_bytes_per_pixel(const struct multiboot_tag_framebuffer_common *fb);
uint32_t mboot_fb_size_bytes    (const struct multiboot_tag_framebuffer_common *fb);
int      mboot_fb_is_rgb        (const struct multiboot_tag_framebuffer_common *fb);
int      mboot_fb_is_text       (const struct multiboot_tag_framebuffer_common *fb);

/* ====================================================================
 * API — Dumps (usam vga_printf se existir; senão são no-op)
 * ==================================================================== */
void mboot_dump     (const struct multiboot_info *mbi);
void mboot_mmap_dump(const struct multiboot_tag_mmap *mmap);

/* ====================================================================
 * ATALHOS DE ITERAÇÃO
 * ==================================================================== */

/* Para cada tag de um tipo específico. Uso:
 *
 *   MBOOT_FOR_EACH_TAG_OF_TYPE(mbi, MULTIBOOT_TAG_TYPE_MODULE, tag) {
 *       struct multiboot_tag_module *m = (void *)tag;
 *       ...
 *   }
 */
#define MBOOT_FOR_EACH_TAG_OF_TYPE(mbi, type_, tag)                             \
    MBOOT_FOR_EACH_TAG(mbi, tag)                                               \
        if ((tag)->type == (type_))

/* Para cada entrada do memory map. Uso:
 *
 *   MBOOT_FOR_EACH_MMAP_ENTRY(mmap, e) {
 *       if (e->type == MULTIBOOT_MEMORY_AVAILABLE) ...
 *   }
 */
#define MBOOT_FOR_EACH_MMAP_ENTRY(mmap, e)                                      \
    for (struct multiboot_mmap_entry *e = (mmap)->entries,                     \
         *_mb_end = (struct multiboot_mmap_entry *)                            \
                     ((uint8_t *)(mmap) + (mmap)->size);                       \
         (uint8_t *)e < (uint8_t *)_mb_end;                                    \
         e = (struct multiboot_mmap_entry *)((uint8_t *)e + (mmap)->entry_size))

/* Para cada módulo carregado. Uso:
 *
 *   struct mboot_module m;
 *   MBOOT_FOR_EACH_MODULE(mbi, i, m) {
 *       // i = índice, m = cópia preenchida
 *   }
 */
#define MBOOT_FOR_EACH_MODULE(mbi, i, m)                                        \
    for (uint32_t i = 0, _mb_n = mboot_get_module_count(mbi);                   \
         i < _mb_n && mboot_read_module_by_number(mbi, i, &(m)) == 0;           \
         i++)

/* ====================================================================
 * ATALHOS DE NOMES (aliases curtos)
 * ==================================================================== */
static inline const char *mboot_cmdline     (const struct multiboot_info *mbi)
    { return mboot_get_cmdline_tag(mbi); }
static inline const char *mboot_bootloader  (const struct multiboot_info *mbi)
    { return mboot_get_bootloader_name_tag(mbi); }
static inline uint32_t    mboot_mem_bytes   (const struct multiboot_info *mbi)
    { return mboot_get_mem_size(mbi); }
static inline uint32_t    mboot_mem_mib     (const struct multiboot_info *mbi)
    { return (mboot_get_mem_size(mbi) + (1u << 20) - 1u) >> 20; }

/* ====================================================================
 * MAIS ONE-LINERS ÚTEIS
 * ==================================================================== */

/* Maior endereço físico disponível (útil para saber até onde mapear). */
uint64_t mboot_mmap_highest_available(const struct multiboot_tag_mmap *mmap);

/* Maior endereço físico de QUALQUER região (inclusive reservada). */
uint64_t mboot_mmap_highest_any(const struct multiboot_tag_mmap *mmap);

/* Endereço físico da primeira região AVAILABLE com tamanho >= size,
 * a partir de min_addr. Devolve 0 se não achar. */
uint64_t mboot_mmap_first_fit(const struct multiboot_tag_mmap *mmap,
                              uint64_t min_addr, uint64_t size);

/* Entrada do mmap que contém 'addr'. NULL se nenhuma. */
struct multiboot_mmap_entry *mboot_mmap_find(const struct multiboot_tag_mmap *mmap,
                                             uint64_t addr);

/* Região AVAILABLE que contém 'addr'? Devolve a entrada ou NULL. */
struct multiboot_mmap_entry *mboot_mmap_find_available(const struct multiboot_tag_mmap *mmap,
                                                       uint64_t addr);

/* Imprime um resumo de UMA linha. Ótimo para o boot. */
void mboot_summary(const struct multiboot_info *mbi);

#endif /* MBOOT_H */