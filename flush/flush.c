#include "colors.h"
#include "command.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char *cwd = getcwd(NULL, 0);
    int last_cmd_res = 0;

    while (1) {
        // TODO: Collect background processes that have terminated (zombies)

        char *color = last_cmd_res >= 0 ? FLUSH_GREEN : FLUSH_RED;
        printf("\n%s%s%s:%s ", FLUSH_CYAN, cwd, color, FLUSH_WHITE);
        fflush(stdout);

        char input[1024];
        input[1023] = '\0';
        ssize_t num_read = read(STDIN_FILENO, input, 1023);

        if (num_read == -1) {
            fprintf(stderr, "flush: could not read from stdin (%s)\n",
                    strerror(errno));
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

        // TODO: Store background process
        last_cmd_res = flush_command_execute(cmd);
    }

    free(cwd);

    return 0;
}
