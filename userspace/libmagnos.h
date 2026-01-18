#ifndef LIBMAGNOS_H
#define LIBMAGNOS_H

/* Syscall numbers */
#define SYSCALL_PRINT   1
#define SYSCALL_EXIT    2

/* Syscall handler function pointer (set by kernel at 0x00100000) */
#define __syscall_handler \
    (*(void (**)(unsigned int, unsigned int, unsigned int, unsigned int))0x00100000)

/* Print a string to the console */
static inline void print(const char *str) {
    void (*handler)(unsigned int, unsigned int, unsigned int, unsigned int) = __syscall_handler;
    if (handler) {
        handler(SYSCALL_PRINT, (unsigned int)str, 0, 0);
    }
}

/* Exit the program */
static inline void exit(int code) {
    void (*handler)(unsigned int, unsigned int, unsigned int, unsigned int) = __syscall_handler;
    if (handler) {
        handler(SYSCALL_EXIT, (unsigned int)code, 0, 0);
    }
}

#endif /* LIBMAGNOS_H */
