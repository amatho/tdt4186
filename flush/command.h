#include <stddef.h>
#include <stdlib.h>

typedef struct {
    char *name;
    char **arguments;
} command_t;

command_t flush_command_parse(char *);

pid_t flush_command_execute(command_t);