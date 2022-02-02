#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    time_t time;
    char *name;
    int pid;
} alarm_t;

typedef struct {
    alarm_t *alarms;
    uintptr_t size;
} alarmlist_t;

void prompt(char *input) {
    printf("> ");
    scanf("%1s", input);
}

int main() {
    time_t current_time = time(NULL);
    printf("Welcome to the alarm clock! It is currently %s\nPlease enter 's' "
           "(schedule), 'l' (list), 'c' (cancel), 'x' (exit)\n",
           ctime(&current_time));

    char input[2];
    while (strcmp(input, "x") != 0) {
        prompt(input);
        if (strcmp(input, "s") == 0) {
            printf("Schedule\n");
        } else if (strcmp(input, "l") == 0) {
            printf("List\n");
        } else if (strcmp(input, "c") == 0) {
            printf("Cancel\n");
        } else if (strcmp(input, "x") == 0) {
            printf("Exit\n");
        } else {
            printf("%s is not a valid input \nPlease enter 's' "
                   "(schedule), 'l' (list), 'c' (cancel), 'x' (exit)\n",
                   input);
        }
    }

    return 0;
}