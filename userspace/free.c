#include "libmagnos.h"

static void uint_to_str(unsigned int val, char *buf) {
    char tmp[12];
    int i = 0;

    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (val > 0) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }

    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

int main(void) {
    unsigned int free_pages = meminfo(0);
    unsigned int total_pages = meminfo(1);
    unsigned int page_size = meminfo(2);
    unsigned int used_pages = total_pages - free_pages;

    unsigned int total_kb = total_pages * (page_size / 1024);
    unsigned int free_kb = free_pages * (page_size / 1024);
    unsigned int used_kb = used_pages * (page_size / 1024);

    char buf[16];

    print("Memory:\n");
    print("  Total: ");
    uint_to_str(total_kb, buf);
    print(buf);
    print(" KB\n");

    print("  Used:  ");
    uint_to_str(used_kb, buf);
    print(buf);
    print(" KB\n");

    print("  Free:  ");
    uint_to_str(free_kb, buf);
    print(buf);
    print(" KB\n");

    print("  Pages: ");
    uint_to_str(free_pages, buf);
    print(buf);
    print("/");
    uint_to_str(total_pages, buf);
    print(buf);
    print(" free\n");

    return 0;
}
