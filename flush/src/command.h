#ifndef _COMMAND_H
#define _COMMAND_H

#include "gvec.h"
#include <stdbool.h>
#include <sys/types.h>

typedef struct {
    char *name;
    gvec_str_t arguments;
    bool is_background;
} command_t;

command_t flush_command_parse(char *);

pid_t flush_command_execute(command_t, gvec_int_t *, gvec_strvec_t *);

#endif
