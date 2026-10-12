#include "upm.h"
#include "elf_loader.h"
#include "paging.h"
#include "tss.h"
#include "usermode.h"
#include "heap.h"
#include "../libs/string.h"
#include "../drivers/serial.h"

static process_t      *g_head       = 0;
static process_t      *g_current    = 0;
static uint32_t        g_next_pid   = 1;
static upm_exit_hook_t g_exit_hook  = 0;

void upm_init(void)
{
    g_head = 0;
    g_current = 0;
    g_next_pid = 1;
    g_exit_hook = 0;
    serial_print("[upm] init\n");
}

void upm_set_exit_hook(upm_exit_hook_t hook) { g_exit_hook = hook; }
process_t *upm_current(void) { return g_current; }

process_t *upm_find(uint32_t pid)
{
    for (process_t *p = g_head; p; p = p->next)
        if (p->pid == pid) return p;
    return 0;
}

static const char *state_name(proc_state_t s)
{
    switch (s) {
        case PROC_LOW:     return "low";
        case PROC_RUNNING: return "running";
        case PROC_PAUSED:  return "paused";
        case PROC_EXITED:  return "exited";
    }
    return "?";
}

static const char *type_name(proc_type_t t)
{
    switch (t) {
        case PROC_PROGRAM: return "program";
        case PROC_SERVICE: return "service";
    }
    return "?";
}

void upm_list(void)
{
    serial_print("PID  TYPE      STATE     NAME\n");
    serial_print("---  --------  --------  ----\n");
    for (process_t *p = g_head; p; p = p->next) {
        serial_print_dec(p->pid);
        serial_print("    ");
        const char *t = type_name(p->type);
        serial_print(t);
        for (int i = strlen(t); i < 8; i++) serial_putc(' ');
        serial_print("  ");
        const char *s = state_name(p->state);
        serial_print(s);
        for (int i = strlen(s); i < 8; i++) serial_putc(' ');
        serial_print("  ");
        serial_print(p->name);
        serial_print("\n");
    }
}

process_t *upm_create(const char *name, proc_type_t type,
                      const void *elf_data, uint32_t elf_size)
{
    if (!elf_data || elf_size == 0) return 0;

    loaded_app_t app;
    if (load_elf32((const uint8_t *)elf_data, elf_size, &app) != 0) {
        serial_print("[upm] load_elf32 falhou\n");
        return 0;
    }

    serial_print("[upm] app: entry=");
    serial_print_hex32(app.entry);
    serial_print(" pd=");
    serial_print_hex32(app.page_dir);
    serial_print(" stack=");
    serial_print_hex32(app.stack_top);
    serial_print("\n");

    process_t *p = (process_t *)heap_alloc(sizeof(process_t), 16);
    if (!p) return 0;

    memset(p, 0, sizeof(process_t));
    p->pid       = g_next_pid++;
    p->type      = type;
    p->state     = PROC_LOW;
    p->page_dir  = app.page_dir;
    p->entry     = app.entry;
    p->stack_top = app.stack_top;
    p->image_end = app.image_end;
    p->kernel_stack_top = (uint32_t)(p->kernel_stack + sizeof(p->kernel_stack));

    uint32_t i = 0;
    while (name[i] && i < UPM_MAX_NAME - 1) { p->name[i] = name[i]; i++; }
    p->name[i] = 0;

    p->next = g_head;
    g_head  = p;

    serial_print("[upm] criado pid=");
    serial_print_dec(p->pid);
    serial_print(" '");
    serial_print(p->name);
    serial_print("'\n");
    return p;
}

void upm_pause(uint32_t pid)
{
    process_t *p = upm_find(pid);
    if (!p || p->state == PROC_EXITED) return;
    p->state = PROC_PAUSED;
}

void upm_resume(uint32_t pid)
{
    process_t *p = upm_find(pid);
    if (!p || p->state != PROC_PAUSED) return;
    p->state = PROC_LOW;
}

void upm_kill(uint32_t pid)
{
    process_t *p = upm_find(pid);
    if (!p) return;
    p->state = PROC_EXITED;
}

void upm_run(process_t *p)
{
    if (!p || p->state == PROC_EXITED || p->state == PROC_PAUSED) return;

    g_current = p;
    p->state  = PROC_RUNNING;

    serial_print("[upm] rodando pid=");
    serial_print_dec(p->pid);
    serial_print(" (");
    serial_print(p->name);
    serial_print(")\n");

    paging_switch_directory(p->page_dir);
    tss_set_kernel_stack(p->kernel_stack_top);
        serial_print("[upm] run: entry=");
    serial_print_hex32(p->entry);
    serial_print(" stack=");
    serial_print_hex32(p->stack_top);
    serial_print(" pd=");
    serial_print_hex32(p->page_dir);
    serial_print("\n");
    enter_user_mode(p->entry, p->stack_top);
}

void upm_exit(int32_t code)
{
    process_t *p = g_current;
    if (p) {
        p->state = PROC_EXITED;
        p->exit_code = code;
    }

    /* Volta pro PD do kernel */
    paging_switch_directory(paging_kernel_directory());

    if (g_exit_hook) g_exit_hook(p, code);

    serial_print("[upm] sem exit_hook — halt\n");
    for (;;) __asm__ volatile ("cli; hlt");
}

void upm_destroy(process_t *p)
{
    if (!p) return;
    process_t **pp = &g_head;
    while (*pp && *pp != p) pp = &(*pp)->next;
    if (*pp == p) *pp = p->next;
    if (g_current == p) g_current = 0;
    if (p->page_dir) paging_destroy_directory(p->page_dir);
    heap_free(p);
}