#include "command.h"
#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum {
    TOKEN_NONE,
    TOKEN_IDENT,
    TOKEN_DQUOTE,
    TOKEN_SQUOTE,
} tokentype_t;

command_t flush_command_parse(char *str) {
    gvec_str_t arguments = {0};
    gvec_str_init(&arguments, 4);

    char *token_start = NULL;
    tokentype_t token_type = TOKEN_NONE;
    size_t str_buf_len = strlen(str) + 1;
    for (size_t i = 0; i < str_buf_len; i++) {
        char curr = str[i];

        if (token_type == TOKEN_NONE) {
            if (curr == '\0') {
                break;
            } else if (curr == '"') {
                token_start = str + i + 1;
                token_type = TOKEN_DQUOTE;
            } else if (curr == '\'') {
                token_start = str + i + 1;
                token_type = TOKEN_SQUOTE;
            } else if (curr != ' ' && curr != '\t') {
                token_start = str + i;
                token_type = TOKEN_IDENT;
            } else if (curr == '\\') {
                continue;
            }
        } else if ((token_type == TOKEN_DQUOTE || token_type == TOKEN_SQUOTE) &&
                   curr == '\\') {
            continue;
        } else if ((token_type == TOKEN_DQUOTE && curr == '"') ||
                   (token_type == TOKEN_SQUOTE && curr == '\'') ||
                   (token_type == TOKEN_IDENT &&
                    (curr == ' ' || curr == '\t' || curr == '\0'))) {
            str[i] = '\0';
            gvec_str_push(&arguments, token_start);
            token_type = TOKEN_NONE;
        }
    }

    if (token_type != TOKEN_NONE) {
        fprintf(stderr,
                "flush: %swarning: multi-line quotes are not supported%s\n",
                FLUSH_RED, FLUSH_WHITE);
    }

    command_t cmd = {0};
    if (arguments.len > 0) {
        bool is_background =
            strcmp(arguments.buf[arguments.len - 1], "&") == 0 ? 1 : 0;
        cmd = (command_t){.name = arguments.buf[0],
                          .arguments = arguments,
                          .is_background = is_background};
    }

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

pid_t flush_command_execute(command_t cmd, gvec_int_t *background_procs,
                            gvec_strvec_t *proc_cmdlines) {
    if (cmd.name == NULL) {
        return 0;
    }

    if (strcmp(cmd.name, "cd") == 0) {
        char *dir = gvec_str_get(&cmd.arguments, 1);
        if (chdir(dir) == -1) {
            fprintf(stderr, "cd: no such directory: %s\n", dir);
            return -1;
        }
    } else if (strcmp(cmd.name, "exit") == 0) {
        exit(EXIT_SUCCESS);
    } else if (strcmp(cmd.name, "jobs") == 0) {
        for (size_t i = 0; i < background_procs->len; i++) {
            gvec_str_t cmdline = proc_cmdlines->buf[i];
            printf("[%d] %s", background_procs->buf[i], cmdline.buf[0]);
            for (size_t j = 1; j < cmdline.len; j++) {
                printf(" %s", cmdline.buf[j]);
            }
            printf("\n");
        }
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
                _exit(EXIT_FAILURE);
            }
        }

        undo_redirect(redir_out, redir_in);
        if (is_background) {
            printf("flush: running %s with PID %d in background\n", cmd.name,
                   pid);
            return pid;
        } else {
            int cmd_status = 0;
            waitpid(pid, &cmd_status, 0);
            char *color = cmd_status == 0 ? FLUSH_GREEN : FLUSH_RED;
            printf("%sExit status (%s) = %d%s\n", color, cmd.name,
                   WEXITSTATUS(cmd_status), FLUSH_WHITE);

            return cmd_status == 0 ? 0 : -1;
        }
    }

    return 0;
}
