#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCESSES 16

typedef enum {
    PROC_UNUSED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_TERMINATED
} proc_state_t;

typedef struct {
    uint32_t pid;
    char name[16];
    proc_state_t state;

    /* Register context (saved during context switch) */
    uint32_t esp;
    uint32_t eip;
    uint32_t page_directory;  /* CR3 for per-process paging */

    /* Kernel stack allocated via pmm_alloc */
    uint32_t kernel_stack;
} process_t;

/* Initialize process subsystem (creates PID 0 = kernel) */
void process_init(void);

/* Create a new process, returns pid or -1 on failure */
int process_create(const char *name, uint32_t entry);

/* Get current running process */
process_t *process_get_current(void);

/* Get process by pid */
process_t *process_get(uint32_t pid);

#endif /* PROCESS_H */
