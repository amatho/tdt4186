#include "gvec.h"
#include <sys/types.h>

typedef struct {
    char *name;
    gvec_str_t arguments;
} command_t;

command_t flush_command_parse(char *);

pid_t flush_command_execute(command_t);
