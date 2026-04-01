#ifndef ARGS_H
#define ARGS_H

/* Program arguments management */
#define MAX_ARGS 16
#define MAX_ARG_LEN 64

typedef struct {
    char args[MAX_ARGS][MAX_ARG_LEN];
    int count;
} program_args_t;

/* Global program arguments (defined in kernel.c) */
extern program_args_t current_program_args;

/* Parse command line into program name and arguments */
void parse_command_line(const char *cmdline, char *program, program_args_t *args);

#endif /* ARGS_H */
