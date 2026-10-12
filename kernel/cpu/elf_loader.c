#include "elf_loader.h"
#include "../elf.h"
#include "paging.h"
#include "pmm.h"
#include "../libs/string.h"
#include "../drivers/serial.h"

#define PHYS_TO_VIRT(p)  ((void *)(uintptr_t)(p))
#define PAGE_SIZE       0x1000
#define PAGE_MASK       0xFFFFF000
#define USER_STACK_TOP  0xBFFFF000
#define USER_STACK_SIZE 0x2000

#define PG_PRESENT 0x01
#define PG_WRITE   0x02
#define PG_USER    0x04

static int elf_validate(const Elf32_Ehdr *eh, uint32_t size)
{
    if (size < sizeof(Elf32_Ehdr))                   return -1;
    if (*(const uint32_t *)eh->e_ident != ELF_MAGIC) return -2;
    if (eh->e_ident[4] != ELFCLASS32)                return -3;
    if (eh->e_machine   != EM_386)                   return -4;
    if (eh->e_type      != 2)                        return -5;
    if (eh->e_phnum     == 0)                        return -6;
    if (eh->e_phentsize != sizeof(Elf32_Phdr))       return -7;
    if (eh->e_phoff + eh->e_phnum * sizeof(Elf32_Phdr) > size)
        return -8;
    return 0;
}

static int load_segment(const uint8_t *elf, uint32_t elf_size,
                        const Elf32_Phdr *ph, uint32_t pd,
                        uint32_t *highest_end)
{
    if (ph->p_offset + ph->p_filesz > elf_size) return -1;
    if (ph->p_memsz  < ph->p_filesz)            return -2;
    if (ph->p_memsz == 0)                       return 0;

    uint32_t vstart = ph->p_vaddr & PAGE_MASK;
    uint32_t vend   = (ph->p_vaddr + ph->p_memsz + PAGE_SIZE - 1) & PAGE_MASK;

    uint32_t pg_flags = PG_PRESENT | PG_USER;
    if (ph->p_flags & PF_W) pg_flags |= PG_WRITE;

    for (uint32_t v = vstart; v < vend; v += PAGE_SIZE) {
        void *frame = pmm_alloc_frame();
        if (!frame) return -3;

        uint8_t *dst = (uint8_t *)PHYS_TO_VIRT((uint32_t)frame);
        memset(dst, 0, PAGE_SIZE);

        uint32_t cstart = (v > ph->p_vaddr) ? v : ph->p_vaddr;
        uint32_t cend   = v + PAGE_SIZE;
        uint32_t segend = ph->p_vaddr + ph->p_filesz;
        if (cend > segend) cend = segend;

        if (cend > cstart) {
            uint32_t off_frame = cstart - v;
            uint32_t off_file  = ph->p_offset + (cstart - ph->p_vaddr);
            memcpy(dst + off_frame, elf + off_file, cend - cstart);
        }

        paging_map_page_in(pd, v, (uint32_t)frame, pg_flags);
    }

    uint32_t end = ph->p_vaddr + ph->p_memsz;
    if (end > *highest_end) *highest_end = end;

    return 0;
}

static int setup_user_stack(uint32_t pd)
{
    uint32_t flags = PG_PRESENT | PG_WRITE | PG_USER;
    for (uint32_t v = USER_STACK_TOP - USER_STACK_SIZE;
         v < USER_STACK_TOP; v += PAGE_SIZE)
    {
        void *frame = pmm_alloc_frame();
        if (!frame) return -1;
        memset(PHYS_TO_VIRT((uint32_t)frame), 0, PAGE_SIZE);
        paging_map_page_in(pd, v, (uint32_t)frame, flags);
    }
    return 0;
}

int load_elf32(const uint8_t *elf, uint32_t elf_size, loaded_app_t *out)
{
    if (!elf || !out) return -1;

    const Elf32_Ehdr *eh = (const Elf32_Ehdr *)elf;
    int err = elf_validate(eh, elf_size);
    if (err) return err;

    serial_print("[elf] e_entry=");
    serial_print_hex32(eh->e_entry);
    serial_print(" e_phoff=");
    serial_print_hex32(eh->e_phoff);
    serial_print(" e_phnum=");
    serial_print_dec(eh->e_phnum);
    serial_print("\n");

    uint32_t pd = paging_create_address_space();
    if (!pd) return -10;

    uint32_t highest = 0;

    for (int i = 0; i < eh->e_phnum; i++) {
        const Elf32_Phdr *ph = (const Elf32_Phdr *)
            (elf + eh->e_phoff + i * eh->e_phentsize);
        if (ph->p_type != PT_LOAD) continue;

        err = load_segment(elf, elf_size, ph, pd, &highest);
        if (err) return err;
    }

    if (setup_user_stack(pd) != 0) return -11;

    out->entry     = eh->e_entry;
    out->page_dir  = pd;
    out->stack_top = USER_STACK_TOP;
    out->image_end = (highest + PAGE_SIZE - 1) & PAGE_MASK;
    return 0;
}