#ifndef _COMMAND_H
#define _COMMAND_H

#include "gvec.h"
#include <stdbool.h>
#include <sys/types.h>

// A parsed command
typedef struct {
    // The command line input as a character vector
    //
    // Invariant: Each argument is separated by a null byte ('\0'), and the
    // command line is terminated by a double null byte.
    gvec_char_t cmdline;
    // Number of arguments in the command line (including the command name)
    size_t argc;
    // Whether the command should be ran in the background or not
    bool is_background;
} command_t;

// Prints the command line, with arguments separated by a space, without a
// newline
//
// cmdline must be a command line from a parsed command.
void flush_print_command_line(char *cmdline);

// Parses str and returns an initialized command_t struct
//
// The caller should destroy the cmdline vector in the struct.
command_t flush_command_parse(char *str);

// Executes cmd, and saves its PID and command line to procs and proc_cmdlines
// if cmd is ran in the background
pid_t flush_command_execute(command_t cmd, gvec_int_t *procs,
                            gvec_str_t *proc_cmdlines);

#endif
