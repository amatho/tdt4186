#include "command.h"
#include "colors.h"
#include "gvec.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum {
    FIND_TOKEN_START,
    PARSE_IDENT,
    PARSE_DOUBLE_QUOTE,
    PARSE_SINGLE_QUOTE,
} parser_state_t;

command_t flush_command_parse(char *str) {
    command_t cmd = {0};

    // Parse the command using a simple state machine
    parser_state_t parser_state = FIND_TOKEN_START;
    size_t str_len = strlen(str);
    size_t cursor = 0;
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
            str[cursor++] = curr;
            parser_state = PARSE_IDENT;
            break;

        case PARSE_IDENT:
            if (curr == ' ' || curr == '\t') {
                // End of identifier
                str[cursor++] = ' ';
                cmd.argc++;
                parser_state = FIND_TOKEN_START;
                break;
            }

            str[cursor++] = curr;
            break;

        case PARSE_DOUBLE_QUOTE:
            if (curr == '"') {
                // End of double quote
                str[cursor++] = ' ';
                cmd.argc++;
                parser_state = FIND_TOKEN_START;
                break;
            }

            str[cursor++] = curr;
            break;

        case PARSE_SINGLE_QUOTE:
            if (curr == '\'') {
                // End of single quote
                str[cursor++] = ' ';
                cmd.argc++;
                parser_state = FIND_TOKEN_START;
                break;
            }

            str[cursor++] = curr;
            break;
        }
    }

    if (cursor == 0) {
        return cmd;
    }

    switch (parser_state) {
    case PARSE_DOUBLE_QUOTE:
    case PARSE_SINGLE_QUOTE:
        fprintf(stderr,
                "flush: %swarning: multi-line quotes are not supported%s\n",
                FLUSH_RED, FLUSH_WHITE);
        // Fall through
    case PARSE_IDENT:
        str[cursor] = '\0';
        cmd.argc++;
        break;
    case FIND_TOKEN_START:
        str[--cursor] = '\0';
        break;
    }

    if (cmd.argc > 1 && str[cursor - 1] == '&') {
        cmd.is_background = 1;
        str[cursor - 2] = '\0';
        cmd.argc--;
    }

    cmd.cmdline = str;
    return cmd;
}

pid_t flush_command_execute(command_t cmd, gvec_int_t *procs,
                            gvec_str_t *proc_cmdlines) {
    if (cmd.argc == 0) {
        return 0;
    }

    char *cmdline = strdup(cmd.cmdline);
    char *cmd_name = strtok(cmdline, " ");
    if (strcmp(cmd_name, "cd") == 0) {
        char *dir = strtok(NULL, " ");
        if (chdir(dir) == -1) {
            fprintf(stderr, "cd: no such directory: %s\n", dir);
            return -1;
        }
    } else if (strcmp(cmd_name, "exit") == 0) {
        exit(EXIT_SUCCESS);
    } else if (strcmp(cmd_name, "jobs") == 0) {
        for (size_t i = 0; i < procs->len; i++) {
            printf("[%d] %s\n", procs->buf[i], proc_cmdlines->buf[i]);
        }
    } else {
        int saved_stdin = -1;
        int fd_in = -1;
        int saved_stdout = -1;
        int fd_out = -1;
        char *arguments[cmd.argc + 1];
        arguments[0] = cmd_name;
        for (size_t i = 1; i < cmd.argc; i++) {
            char *arg = strtok(NULL, " ");
            if (strcmp(arg, "<") == 0 && i < cmd.argc - 1) {
                // redir to stdin
                saved_stdin = dup(STDIN_FILENO);
                fd_in = open(strtok(NULL, " "), O_RDONLY);
                fflush(stdin);
                dup2(fd_in, STDIN_FILENO);
                arguments[i] = NULL;
                i++;
            } else if (strcmp(arg, ">") == 0 && i < cmd.argc - 1) {
                // redir to stdout
                saved_stdout = dup(STDOUT_FILENO);
                fd_out = creat(strtok(NULL, " "),
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
            if (execvp(cmd_name, arguments) == -1) {
                fprintf(stderr, "flush: command not found: %s\n", cmd_name);
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

        pid_t result = 0;
        if (cmd.is_background) {
            printf("flush: running %s with PID %d in background\n", cmd_name,
                   pid);
            result = pid;
        } else {
            int cmd_status = 0;
            waitpid(pid, &cmd_status, 0);
            char *color = cmd_status == 0 ? FLUSH_GREEN : FLUSH_RED;
            printf("%sExit status (%s) = %d%s\n", color, cmd_name,
                   WEXITSTATUS(cmd_status), FLUSH_WHITE);

            if (cmd_status != 0) {
                result = -1;
            }
        }

        free(cmdline);
        return result;
    }

    return 0;
}
