#include "libmagnos.h"

static int strncmp(const char *a, const char *b, int n) {
    while (n && *a && *a == *b) {
        a++;
        b++;
        n--;
    }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

/* Helper function to print a character */
static void putchar_custom(char c) {
    char buf[2];
    buf[0] = c;
    buf[1] = '\0';
    print(buf);
}

int main(void) {
    char cmd_buf[64];
    int cmd_pos = 0;

    print("\nMagnOS Shell Started\n");
    print("MagnOS> ");

    /* Main shell loop - never returns */
    while (1) {
        char c = getchar();

        if (c == '\b') {
            /* Backspace - remove character from buffer */
            if (cmd_pos > 0) {
                cmd_pos--;
                print("\b \b");
            }
        } else if (c == '\n') {
            /* Enter - execute command */
            putchar_custom('\n');

            cmd_buf[cmd_pos] = '\0';
            if (cmd_pos > 0) {
                /* Built-in commands */
                if (strncmp(cmd_buf, "clear", sizeof(cmd_buf)) == 0) {
                    clear();
                    cmd_pos = 0;
                    print("MagnOS> ");
                    continue;
                }

                if (strncmp(cmd_buf, "exit", sizeof(cmd_buf)) == 0) {
                    return 0;
                }

                /* Execute the command */
                int result = exec(cmd_buf);

                if (result == -1) {
                    print("Command not found: ");
                    print(cmd_buf);
                    print("\n");
                } else if (result == -2) {
                    print("Error: File too large\n");
                } else if (result == -3) {
                    print("Error: Failed to read file\n");
                } else if (result == -4) {
                    print("Error: Not an ELF binary\n");
                } else if (result == -5) {
                    print("Error: Failed to execute\n");
                }
            }

            cmd_pos = 0;
            print("MagnOS> ");
        } else if (cmd_pos < (int)sizeof(cmd_buf) - 1) {
            /* Regular character - add to buffer */
            cmd_buf[cmd_pos++] = c;
            putchar_custom(c);
        }
    }

    return 0;
}
