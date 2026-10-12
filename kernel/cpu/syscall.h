#ifndef SYSCALL_H
#define SYSCALL_H

#include "isr.h"   /* registers_t */

#define SYS_exit   0
#define SYS_write  1

void syscall_dispatch(registers_t *r);

#endif