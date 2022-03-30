#include "command.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    size_t arg_cap = 2;
    char **arguments = malloc(sizeof(char *) * arg_cap);
    arguments[0] = cmd_name;
    arguments[1] = NULL;

    while (str != NULL) {
        strip_whitespace(&str);
        char *arg = strsep(&str, " \t");

        if (arg != NULL && arg[0] != '\0') {
            arg_cap++;
            arguments = realloc(arguments, sizeof(char *) * arg_cap);
            arguments[arg_cap - 2] = arg;
            arguments[arg_cap - 1] = NULL;
        }
    }

    command_t cmd = {.name = cmd_name, .arguments = arguments};

    return cmd;
}

static void undo_redirect(FILE *restrict redir_out, FILE *restrict redir_in) {
    if (redir_out != NULL) {
        fclose(redir_out);
        freopen("/dev/tty", "w", stdout);
    } else if (redir_in != NULL) {
        fclose(redir_in);
        freopen("/dev/tty", "r", stdin);
    }
}

pid_t flush_command_execute(command_t cmd) {
    if (strcmp(cmd.name, "cd") == 0) {
        if (chdir(cmd.arguments[1]) == -1) {
            return -1;
        }
    } else if (strcmp(cmd.name, "exit") == 0) {
        exit(EXIT_SUCCESS);
    } else {
        FILE *redir_in = NULL;
        FILE *redir_out = NULL;
        bool is_background = 0;
        for (size_t i = 0; cmd.arguments[i] != NULL; i++) {
            if (strcmp(cmd.arguments[i], "<") == 0 &&
                cmd.arguments[i + 1] != NULL) {
                // redir to stdin
                redir_in = freopen(cmd.arguments[i + 1], "r", stdin);
                cmd.arguments[i] = NULL;
                i++;
            } else if (strcmp(cmd.arguments[i], ">") == 0 &&
                       cmd.arguments[i + 1] != NULL) {
                // redir to stdout
                redir_out = freopen(cmd.arguments[i + 1], "w", stdout);
                cmd.arguments[i] = NULL;
                i++;
            } else if (strcmp(cmd.arguments[i], "&") == 0 &&
                       cmd.arguments[i + 1] == NULL) {
                is_background = 1;
            }
        }

        pid_t pid = fork();
        if (pid == 0) {
            if (execvp(cmd.name, cmd.arguments) == -1) {
                fprintf(stderr, "could not execute command '%s'\n", cmd.name);
                return -1;
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

        return pid;
    }

    return 0;
}