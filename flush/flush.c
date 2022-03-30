#include "command.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char *cwd = getcwd(NULL, 0);

    while (1) {
        printf("\n%s: ", cwd);
        fflush(stdout);

        char input[1024];
        input[1023] = '\0';
        ssize_t num_read = read(STDIN_FILENO, input, 1023);

        if (num_read == -1) {
            perror("could not read from stdin");
            exit(EXIT_FAILURE);
        } else if (num_read == 0) {
            break;
        }

        char *newline = strchr(input, '\n');
        if (newline != NULL) {
            *newline = '\0';
        }

        if (input[num_read - 1] == '\n') {
            input[num_read - 1] = '\0';
        }

        command_t cmd = flush_command_parse(input);

        // printf("%s\n", cmd.name);
        // for (int i = 1; cmd.arguments[i] != NULL; i++) {
        //     printf("arg: '%s'\n", cmd.arguments[i]);
        // }

        pid_t pid = flush_command_execute(cmd);
    }

    free(cwd);

    return 0;
}
