#include "command.h"
#include "colors.h"
#include "gvec.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum {
    FIND_TOKEN_START,
    PARSE_IDENT,
    PARSE_DOUBLE_QUOTE,
    PARSE_SINGLE_QUOTE,
} parser_state_t;

void flush_print_command_line(char *cmdline) {
    printf("%s", cmdline);

    char *arg = strchr(cmdline, '\0') + 1;
    while (*arg != '\0') {
        printf(" %s", arg);
        arg = strchr(arg, '\0') + 1;
    }
}

command_t flush_command_parse(char *str) {
    command_t cmd = {0};
    gvec_char_t cmdline = {0};
    gvec_char_init(&cmdline, 32);

    // Parse the command using a simple state machine
    parser_state_t parser_state = FIND_TOKEN_START;
    size_t str_len = strlen(str);
    for (size_t i = 0; i < str_len; i++) {
        char curr = str[i];

        switch (parser_state) {
        case FIND_TOKEN_START:
            if (curr == ' ' || curr == '\t') {
                // Ignore whitespace
                break;
            } else if (curr == '"') {
                parser_state = PARSE_DOUBLE_QUOTE;
                break;
            } else if (curr == '\'') {
                parser_state = PARSE_SINGLE_QUOTE;
                break;
            }

            // Everything other than whitespace and quotes is recognized as an
            // identifier
            gvec_char_push(&cmdline, curr);
            parser_state = PARSE_IDENT;
            break;

        case PARSE_IDENT:
            if (curr == ' ' || curr == '\t') {
                // End of identifier
                gvec_char_push(&cmdline, '\0');
                cmd.argc++;
                parser_state = FIND_TOKEN_START;
                break;
            }

            gvec_char_push(&cmdline, curr);
            break;

        case PARSE_DOUBLE_QUOTE:
            if (curr == '"') {
                // End of double quote
                gvec_char_push(&cmdline, '\0');
                cmd.argc++;
                parser_state = FIND_TOKEN_START;
                break;
            }

            gvec_char_push(&cmdline, curr);
            break;

        case PARSE_SINGLE_QUOTE:
            if (curr == '\'') {
                // End of single quote
                gvec_char_push(&cmdline, '\0');
                cmd.argc++;
                parser_state = FIND_TOKEN_START;
                break;
            }

            gvec_char_push(&cmdline, curr);
            break;
        }
    }

    switch (parser_state) {
    case PARSE_DOUBLE_QUOTE:
    case PARSE_SINGLE_QUOTE:
        fprintf(stderr,
                "flush: %swarning: multi-line quotes are not supported%s\n",
                FLUSH_RED, FLUSH_WHITE);
        // Fall through
    case PARSE_IDENT:
        gvec_char_push(&cmdline, '\0');
        cmd.argc++;
    case FIND_TOKEN_START:
        break;
    }

    if (cmd.argc > 0) {
        if (cmdline.len > 1 && cmdline.buf[cmdline.len - 2] == '&') {
            cmd.is_background = 1;
            cmdline.buf[cmdline.len - 2] = '\0';
            cmd.argc--;
        } else {
            gvec_char_push(&cmdline, '\0');
        }
    }

    cmd.cmdline = cmdline;

    return cmd;
}

static char *cmdarg(char *cmdline, size_t n) {
    char *arg = cmdline;
    for (size_t i = 0; i < n; i++) {
        arg = strchr(arg, '\0') + 1;
    }
    return arg;
}

pid_t flush_command_execute(command_t cmd, gvec_int_t *procs,
                            gvec_str_t *proc_cmdlines) {
    if (cmd.argc == 0) {
        return 0;
    }

    char *cmdline = cmd.cmdline.buf;
    if (strcmp(cmdline, "cd") == 0) {
        char *dir = cmdarg(cmdline, 1);
        if (chdir(dir) == -1) {
            fprintf(stderr, "cd: no such directory: %s\n", dir);
            return -1;
        }
    } else if (strcmp(cmdline, "exit") == 0) {
        exit(EXIT_SUCCESS);
    } else if (strcmp(cmdline, "jobs") == 0) {
        for (size_t i = 0; i < procs->len; i++) {
            printf("[%d] ", procs->buf[i]);
            flush_print_command_line(proc_cmdlines->buf[i]);
            printf("\n");
        }
    } else {
        int saved_stdin = -1;
        int fd_in = -1;
        int saved_stdout = -1;
        int fd_out = -1;
        char *arguments[cmd.argc + 1];
        for (size_t i = 0; i < cmd.argc; i++) {
            char *arg = cmdarg(cmdline, i);
            if (strcmp(arg, "<") == 0 && i < cmd.argc - 1) {
                // redir to stdin
                saved_stdin = dup(STDIN_FILENO);
                fd_in = open(cmdarg(cmdline, i + 1), O_RDONLY);
                fflush(stdin);
                dup2(fd_in, STDIN_FILENO);
                arguments[i] = NULL;
                i++;
            } else if (strcmp(arg, ">") == 0 && i < cmd.argc - 1) {
                // redir to stdout
                saved_stdout = dup(STDOUT_FILENO);
                fd_out = creat(cmdarg(cmdline, i + 1),
                               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                fflush(stdout);
                dup2(fd_out, STDOUT_FILENO);
                arguments[i] = NULL;
                i++;
            } else {
                arguments[i] = arg;
            }
        }
        arguments[cmd.argc] = NULL;

        pid_t pid = fork();
        if (pid == 0) {
            if (execvp(cmdline, arguments) == -1) {
                fprintf(stderr, "flush: command not found: %s\n", cmdline);
                _exit(EXIT_FAILURE);
            }
        }

        if (saved_stdin != -1) {
            fflush(stdin);
            dup2(saved_stdin, STDIN_FILENO);
            close(fd_in);
        }

        if (saved_stdout != -1) {
            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(fd_out);
        }

        if (cmd.is_background) {
            printf("flush: running %s with PID %d in background\n", cmdline,
                   pid);
            return pid;
        } else {
            int cmd_status = 0;
            waitpid(pid, &cmd_status, 0);
            char *color = cmd_status == 0 ? FLUSH_GREEN : FLUSH_RED;
            printf("%sExit status (%s) = %d%s\n", color, cmdline,
                   WEXITSTATUS(cmd_status), FLUSH_WHITE);

            return cmd_status == 0 ? 0 : -1;
        }
    }

    return 0;
}
