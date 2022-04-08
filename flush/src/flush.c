#include "colors.h"
#include "command.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    char *cwd = getcwd(NULL, 0);
    int last_cmd_res = 0;
    gvec_int_t background_procs = {0};
    gvec_strvec_t proc_cmdlines = {0};
    gvec_int_init(&background_procs, 8);
    gvec_strvec_init(&proc_cmdlines, 8);

    while (1) {
        for (size_t i = 0; i < background_procs.len; i++) {
            pid_t proc = background_procs.buf[i];
            int proc_status = 0;
            if (waitpid(proc, &proc_status, WNOHANG) > 0) {
                char *color = proc_status == 0 ? FLUSH_GREEN : FLUSH_RED;
                gvec_str_t cmdline = proc_cmdlines.buf[i];
                printf("%s[%d] Exit status (%s", color, proc, cmdline.buf[0]);
                for (size_t j = 1; j < cmdline.len; j++) {
                    printf(" %s", cmdline.buf[j]);
                }
                printf(") = %d%s\n", WEXITSTATUS(proc_status), FLUSH_WHITE);
            }
        }

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

        last_cmd_res = flush_command_execute(cmd, &background_procs,
        &proc_cmdlines);

        if (cmd.is_background) {
            gvec_int_push(&background_procs, last_cmd_res);

            gvec_str_t cmdline = {0};
            gvec_str_init(&cmdline, 4);
            for (size_t i = 0; cmd.arguments.buf[i] != NULL; i++) {
                char *arg = cmd.arguments.buf[i];
                char *arg_copy = malloc(strlen(arg) + 1);
                strcpy(arg_copy, arg);
                gvec_str_push(&cmdline, arg_copy);
            }
            gvec_strvec_push(&proc_cmdlines, cmdline);
        }

        // Destroy the arguments vector
        gvec_str_destroy(&cmd.arguments);
    }

    free(cwd);

    return 0;
}
