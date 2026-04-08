#include "libmagnos.h"

/* Simple integer to string conversion */
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
    unsigned int ms = uptime();
    unsigned int seconds = ms / 1000;
    unsigned int minutes = seconds / 60;
    unsigned int hours = minutes / 60;

    char buf[16];

    print("Uptime: ");

    if (hours > 0) {
        uint_to_str(hours, buf);
        print(buf);
        print("h ");
    }

    uint_to_str(minutes % 60, buf);
    print(buf);
    print("m ");

    uint_to_str(seconds % 60, buf);
    print(buf);
    print("s\n");

    return 0;
}
