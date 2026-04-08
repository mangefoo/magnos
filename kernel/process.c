#include "process.h"
#include "pmm.h"

static process_t proc_table[MAX_PROCESSES];
static process_t *current_proc = 0;
static uint32_t next_pid = 0;

void process_init(void) {
    /* Zero the process table */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        proc_table[i].state = PROC_UNUSED;
        proc_table[i].pid = 0;
    }

    /* Create PID 0: the kernel process */
    proc_table[0].pid = next_pid++;
    proc_table[0].state = PROC_RUNNING;
    proc_table[0].esp = 0x1F0000;
    proc_table[0].kernel_stack = 0x1F0000;
    proc_table[0].page_directory = 0;

    /* Set name */
    const char *name = "kernel";
    int i;
    for (i = 0; name[i] && i < 15; i++)
        proc_table[0].name[i] = name[i];
    proc_table[0].name[i] = '\0';

    current_proc = &proc_table[0];
}

int process_create(const char *name, uint32_t entry) {
    /* Find a free slot */
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].state == PROC_UNUSED) {
            slot = i;
            break;
        }
    }
    if (slot < 0)
        return -1;

    /* Allocate kernel stack (1 page = 4KB) */
    uint32_t stack_page = pmm_alloc();
    if (stack_page == 0)
        return -1;

    process_t *p = &proc_table[slot];
    p->pid = next_pid++;
    p->state = PROC_READY;
    p->kernel_stack = stack_page;
    p->esp = stack_page + PAGE_SIZE;  /* Stack grows down */
    p->eip = entry;
    p->page_directory = 0;  /* Shared with kernel for now */

    /* Set name */
    int i;
    for (i = 0; name[i] && i < 15; i++)
        p->name[i] = name[i];
    p->name[i] = '\0';

    return (int)p->pid;
}

process_t *process_get_current(void) {
    return current_proc;
}

process_t *process_get(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].state != PROC_UNUSED && proc_table[i].pid == pid)
            return &proc_table[i];
    }
    return 0;
}
