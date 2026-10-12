/* cpu/kernel_stack.c (ou coloque no main.c) */
#include <stdint.h>

/* 8 KiB de stack dedicada para entradas de ring 0.
 * Alinhada em 16 bytes para o compilador não gerar movaps desalinhado. */
static uint8_t kernel_stack[8192] __attribute__((aligned(16)));

uint32_t kernel_stack_top(void) {
    return (uint32_t)(kernel_stack + sizeof(kernel_stack));
}