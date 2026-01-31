#ifndef LIBMAGNOS_H
#define LIBMAGNOS_H

/* Syscall numbers */
#define SYSCALL_PRINT      1
#define SYSCALL_EXIT       2
#define SYSCALL_FILE_OPEN  3
#define SYSCALL_FILE_READ  4
#define SYSCALL_FILE_CLOSE 5
#define SYSCALL_LIST_DIR   6
#define SYSCALL_GET_ARGS   7

/* Directory entry structure (must match kernel definition) */
typedef struct {
    char name[13];
    unsigned int size;
    unsigned char is_directory;
} dirinfo_t;

/* Syscall handler function pointer (set by kernel at 0x00100000) */
#define __syscall_handler \
    (*(unsigned int (**)(unsigned int, unsigned int, unsigned int, unsigned int))0x00100000)

/* Print a string to the console */
static inline int print(const char *str) {
    unsigned int (*handler)(unsigned int, unsigned int, unsigned int, unsigned int) = __syscall_handler;
    if (handler) {
        return (int)handler(SYSCALL_PRINT, (unsigned int)str, 0, 0);
    }
    return -1;
}

/* Exit the program */
static inline void exit(int code) {
    unsigned int (*handler)(unsigned int, unsigned int, unsigned int, unsigned int) = __syscall_handler;
    if (handler) {
        handler(SYSCALL_EXIT, (unsigned int)code, 0, 0);
    }
}

/* Open a file by name */
static inline int file_open(const char *filename) {
    unsigned int (*handler)(unsigned int, unsigned int, unsigned int, unsigned int) = __syscall_handler;
    if (handler) {
        return (int)handler(SYSCALL_FILE_OPEN, (unsigned int)filename, 0, 0);
    }
    return -1;
}

/* Read from currently open file */
static inline int file_read(unsigned char *buffer, unsigned int size) {
    unsigned int (*handler)(unsigned int, unsigned int, unsigned int, unsigned int) = __syscall_handler;
    if (handler) {
        return (int)handler(SYSCALL_FILE_READ, (unsigned int)buffer, size, 0);
    }
    return -1;
}

/* Close currently open file */
static inline int file_close(void) {
    unsigned int (*handler)(unsigned int, unsigned int, unsigned int, unsigned int) = __syscall_handler;
    if (handler) {
        return (int)handler(SYSCALL_FILE_CLOSE, 0, 0, 0);
    }
    return -1;
}

/* List directory contents */
static inline int list_dir(dirinfo_t *entries, unsigned int max_entries) {
    unsigned int (*handler)(unsigned int, unsigned int, unsigned int, unsigned int) = __syscall_handler;
    if (handler) {
        return (int)handler(SYSCALL_LIST_DIR, (unsigned int)entries, max_entries, 0);
    }
    return -1;
}

/* Get argument count */
static inline int get_argc(void) {
    unsigned int (*handler)(unsigned int, unsigned int, unsigned int, unsigned int) = __syscall_handler;
    if (handler) {
        return (int)handler(SYSCALL_GET_ARGS, (unsigned int)-1, 0, 0);
    }
    return 0;
}

/* Get argument by index (returns 0 on success, -1 on error) */
static inline int get_arg(int index, char *buffer, unsigned int buf_size) {
    unsigned int (*handler)(unsigned int, unsigned int, unsigned int, unsigned int) = __syscall_handler;
    if (handler) {
        return (int)handler(SYSCALL_GET_ARGS, (unsigned int)index, (unsigned int)buffer, buf_size);
    }
    return -1;
}

#endif /* LIBMAGNOS_H */
