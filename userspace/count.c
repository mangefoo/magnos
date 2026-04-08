#include "libmagnos.h"

int main(void) {
    for (int i = 1; i <= 5; i++) {
        char buf[2];
        buf[0] = '0' + i;
        buf[1] = '\0';
        print(buf);
        print("\n");
        if (i < 5) {
            sleep(1000);
        }
    }
    return 0;
}
