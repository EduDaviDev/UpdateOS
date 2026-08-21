#ifndef GDT_H
#define GDT_H

#include <stdint.h>

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

void gdt_init(void);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void gdt_update_tss(uint32_t base, uint32_t limit);

#define GDT_NULL_INDEX  0
#define GDT_CODE_INDEX  1
#define GDT_DATA_INDEX  2
#define GDT_TSS_INDEX   3

#define GDT_CODE_SEL    (GDT_CODE_INDEX << 3)
#define GDT_DATA_SEL    (GDT_DATA_INDEX << 3)
#define GDT_TSS_SEL     (GDT_TSS_INDEX << 3)

#endif