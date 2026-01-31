#include "libmagnos.h"

/* Helper function to convert number to string */
static void uint_to_str(unsigned int num, char *buf) {
    if (num == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    char temp[12];
    int i = 0;

    while (num > 0) {
        temp[i++] = '0' + (num % 10);
        num /= 10;
    }

    /* Reverse */
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

/* Helper function to pad string with spaces */
static void print_padded(const char *str, int width) {
    int len = 0;
    while (str[len]) len++;

    print(str);

    for (int i = len; i < width; i++) {
        print(" ");
    }
}

int main(void) {
    dirinfo_t entries[64];  /* Buffer for up to 64 directory entries */
    char size_buf[12];
    int count;

    print("Directory listing:\n");
    print("--------------------------------------------------\n");
    print("Name            Type    Size\n");
    print("--------------------------------------------------\n");

    /* Get directory listing */
    count = list_dir(entries, 64);

    if (count < 0) {
        print("Error: Failed to read directory\n");
        return 1;
    }

    if (count == 0) {
        print("(empty directory)\n");
        return 0;
    }

    /* Display each entry */
    for (int i = 0; i < count; i++) {
        /* Print filename (padded to 16 chars) */
        print_padded(entries[i].name, 16);

        /* Print type */
        if (entries[i].is_directory) {
            print("<DIR>   ");
        } else {
            print("<FILE>  ");
        }

        /* Print size */
        uint_to_str(entries[i].size, size_buf);
        print(size_buf);
        print(" bytes\n");
    }

    print("--------------------------------------------------\n");
    print("Total entries: ");
    uint_to_str(count, size_buf);
    print(size_buf);
    print("\n");

    return 0;
}
