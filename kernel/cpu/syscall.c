#include "syscall.h"
#include "upm.h"
#include "../drivers/serial.h"

/* O app empilha os argumentos antes do int 0x80.
 * O primeiro (topo) fica em [useresp]. */
static uint32_t user_arg(registers_t *r, int idx)
{
    const uint32_t *stack = (const uint32_t *)(uintptr_t)r->useresp;
    return stack[idx];
}

static void sys_write(registers_t *r)
{
    uint32_t ptr = user_arg(r, 0);
    if (!ptr) return;

    const char *s = (const char *)(uintptr_t)ptr;
    while (*s) {
        serial_putc(*s);
        s++;
    }
}

static void sys_exit(registers_t *r)
{
    int32_t status = (int32_t)user_arg(r, 0);
    upm_exit(status);
    /* upm_exit não retorna */
}

void syscall_dispatch(registers_t *r)
{
    switch (r->eax) {
        case SYS_exit:  sys_exit(r);  break;
        case SYS_write: sys_write(r); break;
        default:
            serial_print("[syscall] desconhecida: ");
            serial_print_hex32(r->eax);
            serial_print("\n");
            break;
    }
}