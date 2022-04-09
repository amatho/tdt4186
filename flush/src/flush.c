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
    gvec_int_t procs = {0};
    gvec_str_t proc_cmdlines = {0};
    gvec_int_init(&procs, 8);
    gvec_str_init(&proc_cmdlines, 8);

    while (1) {
        for (size_t i = 0; i < procs.len; i++) {
            pid_t proc = procs.buf[i];
            int proc_status = 0;
            if (waitpid(proc, &proc_status, WNOHANG) > 0) {
                char *color = proc_status == 0 ? FLUSH_GREEN : FLUSH_RED;
                printf("%s[%d] Exit status (", color, proc);
                flush_print_command_line(proc_cmdlines.buf[i]);
                printf(") = %d%s\n", WEXITSTATUS(proc_status), FLUSH_WHITE);

                gvec_int_remove(&procs, i);
                gvec_str_remove(&proc_cmdlines, i);
                i--;
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

        if (input[num_read - 1] == '\n') {
            input[num_read - 1] = '\0';
        }

        command_t cmd = flush_command_parse(input);

        last_cmd_res = flush_command_execute(cmd, &procs, &proc_cmdlines);

        if (cmd.is_background) {
            gvec_int_push(&procs, last_cmd_res);

            size_t buflen = cmd.cmdline.len;
            char *cmdline_copy = malloc(buflen);
            memcpy(cmdline_copy, cmd.cmdline.buf, buflen);
            gvec_str_push(&proc_cmdlines, cmdline_copy);
        }

        // Destroy the command line vector
        gvec_char_destroy(&cmd.cmdline);
    }

    free(cwd);

    return 0;
}
