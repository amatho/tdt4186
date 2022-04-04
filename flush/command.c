#include "command.h"
#include "gpvec.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void strip_whitespace(char **str) {
    while (**str == ' ' || **str == '\t') {
        (*str)++;
    }
}

command_t flush_command_parse(char *str) {
    strip_whitespace(&str);
    char *cmd_name = strsep(&str, " \t");
    if (cmd_name == NULL) {
        return (command_t){.name = NULL, .arguments = NULL};
    }

    gpvec_t arguments;
    gpvec_init(&arguments, 4);
    gpvec_push(&arguments, cmd_name);

    while (str != NULL) {
        strip_whitespace(&str);
        char *arg = strsep(&str, " \t");

        if (arg != NULL && arg[0] != '\0') {
            gpvec_push(&arguments, arg);
        }
    }

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
    if (strcmp(cmd.name, "cd") == 0) {
        char *dir = gpvec_get(&cmd.arguments, 1);
        if (chdir(dir) == -1) {
            printf("cd: no such directory: %s\n", dir);
            return -1;
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
        }

        // execvp expects a NULL-terminated array
        gpvec_push(&cmd.arguments, NULL);

        pid_t pid = fork();
        if (pid == 0) {
            if (execvp(cmd.name, (char **)cmd.arguments.buf) == -1) {
                fprintf(stderr, "could not execute command '%s'\n", cmd.name);
                exit(EXIT_FAILURE);
            }
        }

        undo_redirect(redir_out, redir_in);
        if (is_background) {
            // TODO: Store background process
        } else {
            int cmd_status = 0;
            waitpid(pid, &cmd_status, 0);
            printf("Exit status [%s] = %d\n", cmd.name,
                   WEXITSTATUS(cmd_status));
        }

        // Destroy the arguments vector
        gpvec_destroy(&cmd.arguments);

        return pid;
    }

    return 0;
}
