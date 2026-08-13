#ifndef UEX_H
#define UEX_H

#include <stdint.h>
#include <stdbool.h>

#define UEX_SIGNATURE "UEX"
#define UEX_HEADER_SIZE 11
#define SYSCALL_INT     0x80
#define MAX_SYSCALLS    512

typedef struct {
    char     signature[3];
    uint32_t stack_size;
    uint32_t entry_offset;
} __attribute__((packed)) UEX_Header;

typedef struct {
    uint32_t command;
    void (*function)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
    uint8_t paramcount;
    uint8_t reserved[3];
} Syscall;

extern Syscall Syscalls[MAX_SYSCALLS];

// Função pública para chamar uma syscall
void syscall(uint32_t command, uint32_t param1, uint32_t param2, uint32_t param3,
             uint32_t param4, uint32_t param5, uint32_t param6);

// Macros para facilitar chamadas com menos parâmetros (como no printf)
#define syscall0(cmd) syscall(cmd, 0,0,0,0,0,0)
#define syscall1(cmd, p1) syscall(cmd, p1,0,0,0,0,0)
#define syscall2(cmd, p1, p2) syscall(cmd, p1,p2,0,0,0,0)
#define syscall3(cmd, p1, p2, p3) syscall(cmd, p1,p2,p3,0,0,0)
#define syscall4(cmd, p1, p2, p3, p4) syscall(cmd, p1,p2,p3,p4,0,0)
#define syscall5(cmd, p1, p2, p3, p4, p5) syscall(cmd, p1,p2,p3,p4,p5,0)
#define syscall6(cmd, p1, p2, p3, p4, p5, p6) syscall(cmd, p1,p2,p3,p4,p5,p6)

// Outras funções
int uex_run(const char *filename);
void uex_syscall_handler(void);
void uex_init(void);

#endif