#include "command.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    bool running = 1;
    while (running) {
        char *cwd = getcwd(NULL, 0);

        printf("\n%s: ", cwd);

        char *input = NULL;
        size_t bufsize;
        ssize_t num_read = getline(&input, &bufsize, stdin);

        if (input[num_read - 1] == '\n') {
            input[num_read - 1] = '\0';
        }

        command_t cmd = flush_command_parse(input);

        // printf("%s\n", cmd.name);
        // for (int i = 1; cmd.arguments[i] != NULL; i++) {
        //     printf("arg: '%s'\n", cmd.arguments[i]);
        // }

        pid_t pid = flush_command_execute(cmd);
        if (pid > 0) {
            int cmd_status = 0;
            waitpid(pid, &cmd_status, 0);
        }

        free(cwd);
        free(input);
    }

    return 0;
    }
