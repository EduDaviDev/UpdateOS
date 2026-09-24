#ifndef MBOOT_H
#define MBOOT_H

#include <stdint.h>

/* Magic number que o bootloader passa em EAX */
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

/* Tags de informação Multiboot2 */
#define MULTIBOOT_TAG_TYPE_END               0
#define MULTIBOOT_TAG_TYPE_CMDLINE           1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME  2
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO     4
#define MULTIBOOT_TAG_TYPE_BOOTDEV           5
#define MULTIBOOT_TAG_TYPE_MMAP              6
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER       8

/* Tag genérica */
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

/* Tag de fim */
struct multiboot_tag_end {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

/* Informações básicas de memória */
struct multiboot_tag_basic_meminfo {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;  /* KB de memória baixa (abaixo de 1 MB) */
    uint32_t mem_upper;  /* KB de memória alta (acima de 1 MB) */
} __attribute__((packed));

/* Informação do framebuffer (opcional, se usar modo gráfico) */
struct multiboot_tag_framebuffer {
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

/* Entrada individual do memory map (tag MMAP) */
struct multiboot_mmap_entry {
    uint64_t addr;      /* Endereço base */
    uint64_t len;       /* Comprimento em bytes */
    uint32_t type;      /* 1 = disponível, 3 = ACPI reclaimable, ... */
    uint32_t zero;      /* Reservado (deve ser 0) */
} __attribute__((packed));

/* Tag completa do memory map */
struct multiboot_tag_mmap {
    uint32_t type;              /* = MULTIBOOT_TAG_TYPE_MMAP (6) */
    uint32_t size;              /* Tamanho total desta tag */
    uint32_t entry_size;        /* Tamanho de cada entrada (geralmente 24) */
    uint32_t entry_version;     /* Versão do formato (0) */
    struct multiboot_mmap_entry entries[];
} __attribute__((packed));

/* Estrutura principal de informações Multiboot2 */
struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
    struct multiboot_tag tags[];
} __attribute__((packed));

#endif /* MBOOT_H */