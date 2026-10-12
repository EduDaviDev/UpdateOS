#include "syscall.h"
#include "upm.h"
#include "paging.h"
#include "pmm.h"
#include "heap.h"
#include "../drivers/serial.h"
#include "../drivers/fatfs/ff.h"
#include "../libs/string.h"

/* Lê 1 argumento da stack do user (sem validação).
 * Use com o índice exato que o app empilhou. */
static uint32_t arg(registers_t *r, int idx)
{
    const uint32_t *stack = (const uint32_t *)(uintptr_t)r->useresp;
    return stack[idx];
}

/* Valida que a faixa [p, p+len) está em território de user. */
static int valid_user(uint32_t p, uint32_t len)
{
    if (p == 0) return 0;
    if (p >= 0xC0000000) return 0;
    if (p + len < p) return 0;
    return 1;
}

/* ============================================================
 *  SYS_exit  (0x00) — 1 arg
 * ============================================================ */
static void sys_exit(registers_t *r)
{
    upm_exit((int32_t)arg(r, 0));
}

/* ============================================================
 *  SYS_print (0x01) — 2 args
 *    p1 = char *str
 *    p2 = uint32_t len  (0 = usa strlen)
 * ============================================================ */
static void sys_print(registers_t *r)
{
    uint32_t ptr = arg(r, 0);
    uint32_t len = arg(r, 1);

    if (!valid_user(ptr, 1)) { r->eax = 0; return; }
    const char *s = (const char *)(uintptr_t)ptr;

    if (len == 0) {
        while (*s) { serial_putc(*s); s++; }
    } else {
        if (!valid_user(ptr, len)) { r->eax = 0; return; }
        for (uint32_t i = 0; i < len; i++) serial_putc(s[i]);
    }
    r->eax = len;
}

/* ============================================================
 *  SYS_power (0x05) — 1 arg
 * ============================================================ */
static void sys_power(registers_t *r)
{
    uint32_t op = arg(r, 0);

    if (op == POWER_shutdown || op == POWER_poweroff) {
        __asm__ volatile ("movw $0x604, %%dx; movw $0x2000, %%ax; outw %%ax, %%dx"
                          ::: "ax", "dx");
        for (;;) __asm__ volatile ("cli; hlt");
    }
    if (op == POWER_reboot) {
        __asm__ volatile ("movb $0x06, %%al; movw $0xCF9, %%dx; outb %%al, %%dx"
                          ::: "al", "dx");
        for (;;) __asm__ volatile ("cli; hlt");
    }
    r->eax = (uint32_t)-1;
}

/* ============================================================
 *  User heap (bump allocator por processo)
 *
 *  Região virtual: 0x40000000..0x50000000 (256 MiB)
 *  Cada página é um frame do PMM mapeado com PG_USER|PG_WRITE
 *  no PD do processo atual.
 * ============================================================ */
#define USER_HEAP_BASE 0x40000000u
#define USER_HEAP_END  0x50000000u

/* Quando o UPM virar multitarefa, isso vira campo do process_t. */
static uint32_t g_user_brk = USER_HEAP_BASE;

static uint32_t user_malloc(uint32_t size)
{
    if (size == 0) return 0;

    uint32_t aligned = (size + 0xFFF) & ~0xFFFu;
    if (g_user_brk + aligned > USER_HEAP_END) return 0;

    /* PD atual = o do processo (ainda estamos nele) */
    uint32_t pd = paging_read_cr3();

    uint32_t start = g_user_brk;

    for (uint32_t off = 0; off < aligned; off += 0x1000) {
        void *frame = pmm_alloc_frame();
        if (!frame) return 0;   /* simples: leak do que já mapeou */

        memset(frame, 0, 0x1000);
        paging_map_page_in(pd, g_user_brk + off, (uint32_t)frame,
                           PG_PRESENT | PG_WRITE | PG_USER);
    }

    g_user_brk += aligned;
    return start;
}

static void user_free(uint32_t ptr)
{
    (void)ptr;   /* bump allocator: free é no-op */
}

/* ============================================================
 *  SYS_mem (0x06) — args dependem da op
 * ============================================================ */
static void sys_mem(registers_t *r)
{
    uint32_t op = arg(r, 0);

    switch (op) {
        case MEM_malloc: {
            /* p2 = size */
            uint32_t size = arg(r, 1);
            r->eax = user_malloc(size);
            return;
        }

        case MEM_free: {
            /* p2 = ptr */
            user_free(arg(r, 1));
            r->eax = 0;
            return;
        }

        case MEM_calloc: {
            /* p2 = count, p3 = size */
            uint32_t count = arg(r, 1);
            uint32_t size  = arg(r, 2);
            r->eax = user_malloc(count * size);   /* já zerado */
            return;
        }

        case MEM_realloc: {
            /* p2 = ptr, p3 = new_size */
            /* Bump allocator não realoca — só aloca e o usuário copia. */
            uint32_t new_size = arg(r, 2);
            r->eax = user_malloc(new_size);
            return;
        }

        default:
            r->eax = 0;
            return;
    }
}

/* ============================================================
 *  SYS_fs (0x03) — args dependem da op
 * ============================================================ */
#define MAX_OPEN_FILES 32
static FIL g_fd_table[MAX_OPEN_FILES];
static int g_fd_used[MAX_OPEN_FILES];

static int fd_alloc(void)
{
    for (int i = 0; i < MAX_OPEN_FILES; i++)
        if (!g_fd_used[i]) { g_fd_used[i] = 1; return i + 1; }
    return -1;
}
static FIL *fd_get(int fd)
{
    if (fd <= 0 || fd > MAX_OPEN_FILES) return 0;
    if (!g_fd_used[fd - 1]) return 0;
    return &g_fd_table[fd - 1];
}
static void fd_free(int fd)
{
    if (fd > 0 && fd <= MAX_OPEN_FILES) g_fd_used[fd - 1] = 0;
}

static void sys_fs(registers_t *r)
{
    uint32_t op = arg(r, 0);

    switch (op) {
        case FS_open: {   /* p2 = path, p3 = mode */
            uint32_t path = arg(r, 1);
            uint32_t mode = arg(r, 2);
            if (!valid_user(path, 1)) { r->eax = (uint32_t)-1; return; }
            int fd = fd_alloc();
            if (fd < 0) { r->eax = (uint32_t)-1; return; }
            if (f_open(&g_fd_table[fd-1], (const char *)(uintptr_t)path, (BYTE)mode) != FR_OK) {
                fd_free(fd);
                r->eax = (uint32_t)-1;
                return;
            }
            r->eax = (uint32_t)fd;
            return;
        }

        case FS_close: {   /* p2 = fd */
            int fd = (int)arg(r, 1);
            FIL *f = fd_get(fd);
            if (!f) { r->eax = (uint32_t)-1; return; }
            f_close(f);
            fd_free(fd);
            r->eax = 0;
            return;
        }

        case FS_read: {    /* p2 = fd, p3 = buf, p4 = count */
            int fd = (int)arg(r, 1);
            uint32_t buf = arg(r, 2);
            uint32_t cnt = arg(r, 3);
            FIL *f = fd_get(fd);
            if (!f) { r->eax = (uint32_t)-1; return; }
            if (!valid_user(buf, cnt)) { r->eax = (uint32_t)-1; return; }
            UINT br = 0;
            r->eax = (f_read(f, (void *)(uintptr_t)buf, cnt, &br) == FR_OK)
                   ? br : (uint32_t)-1;
            return;
        }

        case FS_write: {   /* p2 = fd, p3 = buf, p4 = count */
            int fd = (int)arg(r, 1);
            uint32_t buf = arg(r, 2);
            uint32_t cnt = arg(r, 3);
            FIL *f = fd_get(fd);
            if (!f) { r->eax = (uint32_t)-1; return; }
            if (!valid_user(buf, cnt)) { r->eax = (uint32_t)-1; return; }
            UINT bw = 0;
            r->eax = (f_write(f, (const void *)(uintptr_t)buf, cnt, &bw) == FR_OK)
                   ? bw : (uint32_t)-1;
            return;
        }

        case FS_seek: {    /* p2 = fd, p3 = offset */
            FIL *f = fd_get((int)arg(r, 1));
            if (!f) { r->eax = (uint32_t)-1; return; }
            r->eax = (f_lseek(f, (FSIZE_t)arg(r, 2)) == FR_OK)
                   ? 0 : (uint32_t)-1;
            return;
        }

        case FS_tell:      /* p2 = fd */
            r->eax = (uint32_t)(f_tell(fd_get((int)arg(r, 1))) ?: 0);
            return;

        case FS_size:      /* p2 = fd */
            r->eax = (uint32_t)f_size(fd_get((int)arg(r, 1)));
            return;

        case FS_remove:    /* p2 = path */
            r->eax = (f_unlink((const char *)(uintptr_t)arg(r, 1)) == FR_OK)
                   ? 0 : (uint32_t)-1;
            return;

        case FS_mkdir:     /* p2 = path */
            r->eax = (f_mkdir((const char *)(uintptr_t)arg(r, 1)) == FR_OK)
                   ? 0 : (uint32_t)-1;
            return;

        default:
            r->eax = (uint32_t)-1;
            return;
    }
}

/* ============================================================
 *  SYS_run / SYS_gfx — stub
 * ============================================================ */
static void sys_run(registers_t *r)
{
    serial_print("[syscall] sys_run ainda não implementado\n");
    r->eax = (uint32_t)-1;
}

static void sys_gfx(registers_t *r)
{
    r->eax = (uint32_t)-1;
}

/* ============================================================
 *  Dispatcher
 * ============================================================ */
void syscall_dispatch(registers_t *r)
{
    switch (r->eax) {
        case SYS_exit:  sys_exit(r);  return;
        case SYS_print: sys_print(r); return;
        case SYS_run:   sys_run(r);   return;
        case SYS_fs:    sys_fs(r);    return;
        case SYS_gfx:   sys_gfx(r);   return;
        case SYS_power: sys_power(r); return;
        case SYS_mem:   sys_mem(r);   return;
        default:
            serial_print("[syscall] desconhecida: ");
            serial_print_hex32(r->eax);
            serial_print("\n");
            r->eax = (uint32_t)-1;
            return;
    }
}