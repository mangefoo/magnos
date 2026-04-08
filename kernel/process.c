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
    p->eip = entry;
    p->page_directory = 0;  /* Shared with kernel for now */

    /*
     * Build initial stack frame for context_switch:
     * context_switch does popa + sti + ret, so we need:
     *   [top of stack]
     *   entry_point    <- ret pops this (EIP)
     *   edi = 0        <- popa pops these (8 regs, 32 bytes)
     *   esi = 0
     *   ebp = 0
     *   esp = 0 (ignored by popa)
     *   ebx = 0
     *   edx = 0
     *   ecx = 0
     *   eax = 0
     *   [esp points here]
     */
    uint32_t *sp = (uint32_t *)(stack_page + PAGE_SIZE);
    *(--sp) = entry;    /* Return address for ret */
    *(--sp) = 0;        /* EDI */
    *(--sp) = 0;        /* ESI */
    *(--sp) = 0;        /* EBP */
    *(--sp) = 0;        /* ESP (ignored by popa) */
    *(--sp) = 0;        /* EBX */
    *(--sp) = 0;        /* EDX */
    *(--sp) = 0;        /* ECX */
    *(--sp) = 0;        /* EAX */
    p->esp = (uint32_t)sp;

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

void schedule(void) {
    if (!current_proc)
        return;

    /* Find next READY process (round-robin) */
    int current_slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (&proc_table[i] == current_proc) {
            current_slot = i;
            break;
        }
    }

    int next_slot = -1;
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        int idx = (current_slot + i) % MAX_PROCESSES;
        if (proc_table[idx].state == PROC_READY) {
            next_slot = idx;
            break;
        }
    }

    if (next_slot < 0 || next_slot == current_slot)
        return;  /* No other process to switch to */

    process_t *old = current_proc;
    process_t *new = &proc_table[next_slot];

    /* Update states */
    if (old->state == PROC_RUNNING)
        old->state = PROC_READY;
    new->state = PROC_RUNNING;
    current_proc = new;

    context_switch(&old->esp, new->esp);
}

process_t *process_get(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].state != PROC_UNUSED && proc_table[i].pid == pid)
            return &proc_table[i];
    }
    return 0;
}
