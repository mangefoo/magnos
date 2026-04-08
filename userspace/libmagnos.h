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
#define SYSCALL_GETCHAR    8
#define SYSCALL_EXEC       9
#define SYSCALL_CLEAR      10
#define SYSCALL_SLEEP      11
#define SYSCALL_UPTIME     12
#define SYSCALL_MEMINFO    13
#define SYSCALL_HEAP_STATS 14

/* Directory entry structure (must match kernel definition) */
typedef struct {
    char name[13];
    unsigned int size;
    unsigned char is_directory;
} dirinfo_t;

/* Generic syscall via int 0x80 */
static inline unsigned int __syscall(unsigned int num, unsigned int a1,
                                     unsigned int a2, unsigned int a3) {
    unsigned int ret;
    __asm__ volatile("int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(a1), "c"(a2), "d"(a3)
        : "memory");
    return ret;
}

static inline int print(const char *str) {
    return (int)__syscall(SYSCALL_PRINT, (unsigned int)str, 0, 0);
}

static inline void exit(int code) {
    __syscall(SYSCALL_EXIT, (unsigned int)code, 0, 0);
}

static inline int file_open(const char *filename) {
    return (int)__syscall(SYSCALL_FILE_OPEN, (unsigned int)filename, 0, 0);
}

static inline int file_read(unsigned char *buffer, unsigned int size) {
    return (int)__syscall(SYSCALL_FILE_READ, (unsigned int)buffer, size, 0);
}

static inline int file_close(void) {
    return (int)__syscall(SYSCALL_FILE_CLOSE, 0, 0, 0);
}

static inline int list_dir(dirinfo_t *entries, unsigned int max_entries) {
    return (int)__syscall(SYSCALL_LIST_DIR, (unsigned int)entries, max_entries, 0);
}

static inline int get_argc(void) {
    return (int)__syscall(SYSCALL_GET_ARGS, (unsigned int)-1, 0, 0);
}

static inline int get_arg(int index, char *buffer, unsigned int buf_size) {
    return (int)__syscall(SYSCALL_GET_ARGS, (unsigned int)index, (unsigned int)buffer, buf_size);
}

static inline char getchar(void) {
    return (char)__syscall(SYSCALL_GETCHAR, 0, 0, 0);
}

static inline void clear(void) {
    __syscall(SYSCALL_CLEAR, 0, 0, 0);
}

static inline int exec(const char *cmdline) {
    return (int)__syscall(SYSCALL_EXEC, (unsigned int)cmdline, 0, 0);
}

static inline unsigned int meminfo(unsigned int info_type) {
    return __syscall(SYSCALL_MEMINFO, info_type, 0, 0);
}

static inline unsigned int heap_stats(unsigned int info_type) {
    return __syscall(SYSCALL_HEAP_STATS, info_type, 0, 0);
}

static inline void sleep(unsigned int ms) {
    __syscall(SYSCALL_SLEEP, ms, 0, 0);
}

static inline unsigned int uptime(void) {
    return __syscall(SYSCALL_UPTIME, 0, 0, 0);
}

#endif /* LIBMAGNOS_H */
