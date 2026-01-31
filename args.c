#include "args.h"

/* Global storage for current program arguments */
program_args_t current_program_args = {{}, 0};

/* Parse command line into program name and arguments */
void parse_command_line(const char *cmdline, char *program, program_args_t *args) {
    int i = 0;
    int prog_idx = 0;

    /* Clear args */
    args->count = 0;
    for (int j = 0; j < MAX_ARGS; j++) {
        args->args[j][0] = '\0';
    }

    /* Skip leading spaces */
    while (cmdline[i] == ' ') i++;

    /* Extract program name (first word) */
    while (cmdline[i] && cmdline[i] != ' ' && prog_idx < MAX_ARG_LEN - 1) {
        program[prog_idx++] = cmdline[i++];
    }
    program[prog_idx] = '\0';

    /* Parse arguments */
    while (cmdline[i] && args->count < MAX_ARGS) {
        /* Skip spaces */
        while (cmdline[i] == ' ') i++;

        if (!cmdline[i]) break;

        /* Extract argument */
        int arg_idx = 0;
        while (cmdline[i] && cmdline[i] != ' ' && arg_idx < MAX_ARG_LEN - 1) {
            args->args[args->count][arg_idx++] = cmdline[i++];
        }
        args->args[args->count][arg_idx] = '\0';

        if (arg_idx > 0) {
            args->count++;
        }
    }
}
