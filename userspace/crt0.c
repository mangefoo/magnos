/* C runtime startup code for userspace programs */

#include "libmagnos.h"

/* Entry point called by kernel */
void _start(void);

/* User's main function */
extern int main(void);

void _start(void) {
    /* Call user's main */
    int ret = main();

    /* Exit with return code */
    exit(ret);

    /* Hang */
    while (1) {
        __asm__ volatile("hlt");
    }
}
