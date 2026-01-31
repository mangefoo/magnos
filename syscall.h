#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

/* Syscall numbers */
#define SYSCALL_PRINT      1
#define SYSCALL_EXIT       2
#define SYSCALL_FILE_OPEN  3
#define SYSCALL_FILE_READ  4
#define SYSCALL_FILE_CLOSE 5
#define SYSCALL_LIST_DIR   6

/* Syscall handler */
uint32_t syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3);

/* Initialize syscalls */
void syscall_init(void);

#endif /* SYSCALL_H */
