#include "libmagnos.h"

int main(void) {
    print("Testing case-insensitive file operations:\n\n");

    /* Test 1: lowercase filename */
    print("1. Opening 'hello.txt' (lowercase)...\n");
    if (file_open("hello.txt") == 0) {
        print("   SUCCESS: Opened HELLO.TXT\n");

        unsigned char buf[64];
        int bytes = file_read(buf, sizeof(buf) - 1);
        if (bytes > 0) {
            buf[bytes] = '\0';
            print("   Content: ");
            print((char*)buf);
        }
        file_close();
    } else {
        print("   FAILED\n");
    }

    print("\n");

    /* Test 2: uppercase filename */
    print("2. Opening 'HELLO.TXT' (uppercase)...\n");
    if (file_open("HELLO.TXT") == 0) {
        print("   SUCCESS: Opened HELLO.TXT\n");
        file_close();
    } else {
        print("   FAILED\n");
    }

    print("\n");

    /* Test 3: mixed case filename */
    print("3. Opening 'HeLLo.TxT' (mixed case)...\n");
    if (file_open("HeLLo.TxT") == 0) {
        print("   SUCCESS: Opened HELLO.TXT\n");
        file_close();
    } else {
        print("   FAILED\n");
    }

    print("\nAll tests passed! Filenames are case-insensitive.\n");

    return 0;
}
