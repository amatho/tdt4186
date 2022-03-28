#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    bool running = 1;
    while (running) {
        char *cwd = getcwd(NULL, 0);

        printf("%s: ", cwd);

        char *input = NULL;
        size_t bufsize;
        ssize_t num_read = getline(&input, &bufsize, stdin);

        if (input[num_read - 1] == '\n') {
            input[num_read - 1] = '\0';
        }

        printf("%s\n", input);

        if (strcmp(input, "exit") == 0) {
            running = 0;
        }

        free(cwd);
        free(input);
    }

    return 0;
}
