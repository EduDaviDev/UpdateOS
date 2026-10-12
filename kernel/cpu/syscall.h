#ifndef SYSCALL_H
#define SYSCALL_H

#include "isr.h"

/* ============================================================
 *  Syscall numbers (eax)
 * ============================================================ */
#define SYS_exit    0x00
#define SYS_print   0x01
#define SYS_run     0x02
#define SYS_fs      0x03
#define SYS_gfx     0x04
#define SYS_power   0x05
#define SYS_mem     0x06

/* ============================================================
 *  sys_power ops (p1)
 * ============================================================ */
#define POWER_shutdown  0x01
#define POWER_reboot    0x02
#define POWER_poweroff  0x03

/* ============================================================
 *  sys_fs ops (p1)
 * ============================================================ */
#define FS_open     0x01
#define FS_close    0x02
#define FS_read     0x03
#define FS_write    0x04
#define FS_seek     0x05
#define FS_tell     0x06
#define FS_size     0x07
#define FS_rename   0x08
#define FS_remove   0x09
#define FS_mkdir    0x0A
#define FS_rmdir    0x0B
#define FS_chdir    0x0C
#define FS_getcwd   0x0D
#define FS_getstat  0x0E
#define FS_chmod    0x0F
#define FS_stat     0x10

/* ============================================================
 *  sys_mem ops (p1)
 * ============================================================ */
#define MEM_malloc   0x01
#define MEM_free     0x02
#define MEM_calloc   0x03
#define MEM_realloc  0x04

/* ============================================================
 *  sys_gfx ops (p1) — só os que usamos agora
 * ============================================================ */
#define GFX_get_width      0x01
#define GFX_get_height     0x02
#define GFX_get_bpp        0x03
#define GFX_clear          0x10
#define GFX_putpixel       0x11
#define GFX_print          0x17
#define GFX_print_char     0x18

void syscall_dispatch(registers_t *r);

#endif