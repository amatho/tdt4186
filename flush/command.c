#include "command.h"
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

pid_t flush_command_execute(command_t cmd) {
    if (strcmp(cmd.name, "cd") == 0) {
        if (chdir(cmd.arguments[1]) == -1) {
            return -1;
        }
        // TODO: do
    } else if (strcmp(cmd.name, "exit") == 0) {
        exit(EXIT_SUCCESS);
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            if (execvp(cmd.name, cmd.arguments) == -1) {
                fprintf(stderr, "could not execute command '%s'\n", cmd.name);
                return -1;
            }
        }

        return pid;
    }

    return 0;
}