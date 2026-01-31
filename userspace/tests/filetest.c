#include "libmagnos.h"

int main(void) {
    unsigned char buffer[512];
    int result;
    int bytes_read;
    unsigned int i;

    print("FileTest: Opening HELLO.TXT...\n");

    /* Open the file */
    result = file_open("HELLO.TXT");
    if (result != 0) {
        print("FileTest: Failed to open file!\n");
        exit(1);
    }

    print("FileTest: File opened successfully!\n");
    print("FileTest: Reading file contents...\n\n");

    /* Read file contents */
    bytes_read = file_read(buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        print("FileTest: Failed to read file!\n");
        file_close();
        exit(1);
    }

    /* Null-terminate the buffer */
    if (bytes_read < (int)sizeof(buffer)) {
        buffer[bytes_read] = '\0';
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
    }

    /* Print the file contents */
    print("--- File Contents ---\n");
    print((const char *)buffer);
    print("\n--- End of File ---\n\n");

    print("FileTest: Read ");
    /* Simple number to string conversion */
    if (bytes_read >= 100) {
        buffer[0] = '0' + (bytes_read / 100);
        buffer[1] = '0' + ((bytes_read / 10) % 10);
        buffer[2] = '0' + (bytes_read % 10);
        buffer[3] = '\0';
    } else if (bytes_read >= 10) {
        buffer[0] = '0' + (bytes_read / 10);
        buffer[1] = '0' + (bytes_read % 10);
        buffer[2] = '\0';
    } else {
        buffer[0] = '0' + bytes_read;
        buffer[1] = '\0';
    }
    print((const char *)buffer);
    print(" bytes\n");

    /* Close the file */
    result = file_close();
    if (result != 0) {
        print("FileTest: Warning - failed to close file\n");
    } else {
        print("FileTest: File closed successfully\n");
    }

    print("\nFileTest: All tests passed!\n");
    return 0;
}
