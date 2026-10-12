#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include <stdint.h>

typedef struct {
    uint32_t entry;
    uint32_t page_dir;
    uint32_t stack_top;
    uint32_t image_end;
} loaded_app_t;

int load_elf32(const uint8_t *elf, uint32_t elf_size, loaded_app_t *out);

#endif