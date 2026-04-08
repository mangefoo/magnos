#include "libmagnos.h"

int main(void) {
    print("Triggering page fault (writing to 0x2000000 = 32MB)...\n");
    volatile unsigned int *bad_ptr = (volatile unsigned int *)0x2000000;
    *bad_ptr = 0xDEAD;
    print("This should never be reached!\n");
    return 0;
}
