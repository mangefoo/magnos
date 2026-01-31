#include "libmagnos.h"

int main(void) {
    unsigned char buffer[1024];
    char filename[64];
    int argc;
    int result;
    int bytes_read;

    /* Get argument count */
    argc = get_argc();

    /* Check if we have a filename argument */
    if (argc < 1) {
        print("Usage: cat <filename>\n");
        return 1;
    }

    /* Get the filename from arguments */
    if (get_arg(0, filename, sizeof(filename)) != 0) {
        print("Error: Failed to get filename argument\n");
        return 1;
    }

    /* Open the file */
    result = file_open(filename);
    if (result != 0) {
        print("cat: ");
        print(filename);
        print(": No such file\n");
        return 1;
    }

    /* Read and print file contents */
    while (1) {
        bytes_read = file_read(buffer, sizeof(buffer) - 1);

        if (bytes_read < 0) {
            print("cat: Error reading file\n");
            file_close();
            return 1;
        }

        if (bytes_read == 0) {
            /* End of file */
            break;
        }

        /* Null-terminate and print */
        buffer[bytes_read] = '\0';
        print((const char *)buffer);
    }

    /* Close the file */
    file_close();

    return 0;
}
