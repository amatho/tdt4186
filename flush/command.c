#include "command.h"
#include "colors.h"
#include "gvec.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

command_t flush_command_parse(char *str) {
    gvec_str_t arguments;
    gvec_str_init(&arguments, 4);

    char *arg;
    while ((arg = strsep(&str, " \t")) != NULL) {
        if (*arg == '\0') {
            continue;
        }

        gvec_str_push(&arguments, arg);
    }

    char *cmd_name = arguments.len > 0 ? arguments.buf[0] : NULL;

    command_t cmd = {.name = cmd_name, .arguments = arguments};

    return cmd;
}

static void undo_redirect(FILE *restrict redir_out, FILE *restrict redir_in) {
    if (redir_out != NULL) {
        fclose(redir_out);
        freopen("/dev/tty", "w", stdout);
    }

    if (redir_in != NULL) {
        fclose(redir_in);
        freopen("/dev/tty", "r", stdin);
    }
}

pid_t flush_command_execute(command_t cmd) {
    pid_t exec_result;

    if (cmd.name == NULL) {
        exec_result = 0;
    } else if (strcmp(cmd.name, "cd") == 0) {
        char *dir = gvec_str_get(&cmd.arguments, 1);
        if (chdir(dir) == -1) {
            fprintf(stderr, "cd: no such directory: %s\n", dir);
            exec_result = -1;
        }
    } else if (strcmp(cmd.name, "exit") == 0) {
        exit(EXIT_SUCCESS);
    } else {
        FILE *redir_in = NULL;
        FILE *redir_out = NULL;
        for (size_t i = 0; i < cmd.arguments.len; i++) {
            if (strcmp(cmd.arguments.buf[i], "<") == 0 &&
                i < cmd.arguments.len - 1) {
                // redir to stdin
                redir_in = freopen(cmd.arguments.buf[i + 1], "r", stdin);
                cmd.arguments.buf[i] = NULL;
                i++;
            } else if (strcmp(cmd.arguments.buf[i], ">") == 0 &&
                       i < cmd.arguments.len - 1) {
                // redir to stdout
                redir_out = freopen(cmd.arguments.buf[i + 1], "w", stdout);
                cmd.arguments.buf[i] = NULL;
                i++;
            }
        }

        bool is_background = 0;
        if (strcmp(cmd.arguments.buf[cmd.arguments.len - 1], "&") == 0) {
            is_background = 1;
            cmd.arguments.buf[cmd.arguments.len - 1] = NULL;
        } else if (redir_in == NULL && redir_out == NULL) {
            // execvp expects a NULL-terminated array
            gvec_str_push(&cmd.arguments, NULL);
        }

        pid_t pid = fork();
        if (pid == 0) {
            if (execvp(cmd.name, (char **)cmd.arguments.buf) == -1) {
                fprintf(stderr, "flush: command not found: %s\n", cmd.name);
                exit(EXIT_FAILURE);
            }
        }

        undo_redirect(redir_out, redir_in);
        if (is_background) {
            printf("flush: running %s with PID %d in background\n", cmd.name,
                   pid);
            exec_result = pid;
        } else {
            int cmd_status = 0;
            waitpid(pid, &cmd_status, 0);
            char *color = cmd_status == 0 ? FLUSH_GREEN : FLUSH_RED;
            printf("%sExit status [%s] = %d%s\n", color, cmd.name,
                   WEXITSTATUS(cmd_status), FLUSH_WHITE);

            exec_result = cmd_status == 0 ? 0 : -1;
        }
    }

    // Destroy the arguments vector
    gvec_str_destroy(&cmd.arguments);

    return exec_result;
}
