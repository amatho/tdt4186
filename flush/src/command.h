#ifndef _COMMAND_H
#define _COMMAND_H

#include "gvec.h"
#include <stdbool.h>
#include <sys/types.h>

// A parsed command
typedef struct {
    // The command line string
    char *cmdline;
    // Number of arguments in the command line (including the command name)
    size_t argc;
    // Whether the command should be ran in the background or not
    bool is_background;
} command_t;

// Parses str and returns an initialized command_t struct.
//
// The caller should destroy the cmdline vector in the struct.
command_t flush_command_parse(char *str);

// Executes the given cmd.
//
// procs and proc_cmdlines are used to print backround processes.
pid_t flush_command_execute(command_t cmd, gvec_int_t *procs,
                            gvec_str_t *proc_cmdlines);

#endif
