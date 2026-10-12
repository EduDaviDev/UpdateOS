#ifndef UPM_H
#define UPM_H

#include <stdint.h>

typedef enum {
    PROC_PROGRAM = 0,
    PROC_SERVICE = 1,
} proc_type_t;

typedef enum {
    PROC_LOW     = 0,
    PROC_RUNNING = 1,
    PROC_PAUSED  = 2,
    PROC_EXITED  = 3,
} proc_state_t;

#define UPM_MAX_NAME 32

typedef struct process {
    uint32_t      pid;
    char          name[UPM_MAX_NAME];
    proc_type_t   type;
    proc_state_t  state;

    uint32_t      page_dir;
    uint32_t      entry;
    uint32_t      stack_top;
    uint32_t      image_end;

    uint32_t      kernel_stack_top;
    uint8_t       kernel_stack[8192] __attribute__((aligned(16)));

    int32_t       exit_code;

    struct process *next;
} process_t;

void         upm_init(void);

process_t   *upm_create(const char *name, proc_type_t type,
                        const void *elf_data, uint32_t elf_size);

process_t   *upm_find(uint32_t pid);
process_t   *upm_current(void);
void         upm_list(void);

void         upm_pause (uint32_t pid);
void         upm_resume(uint32_t pid);
void         upm_kill  (uint32_t pid);

void         upm_run(process_t *p);
void         upm_exit(int32_t code) __attribute__((noreturn));

typedef void (*upm_exit_hook_t)(process_t *p, int32_t code);
void         upm_set_exit_hook(upm_exit_hook_t hook);

void         upm_destroy(process_t *p);

#endif