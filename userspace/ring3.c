#include "libmagnos.h"

int main(void) {
    print("Running in ring 3 (user mode)\n\n");

    print("Test 1: Syscalls work from ring 3\n");
    print("  print() via int 0x80: OK\n");

    print("\nTest 2: Reading uptime via syscall\n");
    unsigned int ms = uptime();
    char buf[16];
    char tmp[12];
    int i = 0;
    unsigned int val = ms / 1000;
    if (val == 0) { tmp[i++] = '0'; }
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
    print("  Uptime: ");
    print(buf);
    print("s OK\n");

    print("\nTest 3: Attempting to read kernel memory (0x1000)...\n");
    print("  This should cause a page fault:\n");
    volatile unsigned int *kernel_mem = (volatile unsigned int *)0x1000;
    unsigned int val2 = *kernel_mem;
    (void)val2;

    print("  ERROR: Should not reach here!\n");
    return 0;
}
