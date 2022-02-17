#define _XOPEN_SOURCE

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// A single alarm
typedef struct {
    time_t time;
    pid_t pid;
} alarm_t;

// A simple list data structure for the alarms
typedef struct {
    alarm_t *alarms;
    uintptr_t size;
} alarmlist_t;

// Strips the trailing newline of a string, if there is one
void strip_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

// Schedules an alarm based on user input
void schedule(alarmlist_t *alarmlist) {
    char input[32];
    struct tm date;

    // Ask for new input until valid input is given
    do {
        printf("Schedule alarm at which date and time? ");
        fgets(input, 32, stdin);
        strip_newline(input);
    } while (strptime(input, "%Y-%m-%d %H:%M:%S", &date) == NULL);

    time_t alarm_time = mktime(&date);

    // Creates a child process
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Could not spawn a child process");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process waits the given amount of time before ringing
        unsigned int df = (unsigned int)difftime(alarm_time, time(NULL));
        sleep(df);

#if __APPLE__
        execlp("afplay", "afplay", "-t", "20", "ringtone.mp3", NULL);
#elif __linux__
        execlp("mpg123", "mpg123", "-q", "-n", "775", "ringtone.mp3", NULL);
#else
#error "Unsupported system"
#endif

        _exit(EXIT_FAILURE);
    } else {
        // In the parent process, add the alarm to the list
        alarm_t alarm = {.time = alarm_time, .pid = pid};
        alarmlist->size += 1;
        alarmlist->alarms =
            realloc(alarmlist->alarms, sizeof(alarm_t) * (alarmlist->size));
        alarmlist->alarms[alarmlist->size - 1] = alarm;
    }
}

// Lists the scheduled alarms
void list(alarmlist_t alarmlist) {
    if (alarmlist.size == 0) {
        printf("There are no alarms.\n");
        return;
    }

    printf("Alarms:\n");

    for (int i = 0; i < alarmlist.size; i++) {
        alarm_t a = alarmlist.alarms[i];
        printf("- Alarm %d: %s", i + 1, ctime(&a.time));
    }
}

// Cancels a scheduled alarm
void cancel(alarmlist_t *alarmlist) {
    if (alarmlist->size == 0) {
        printf("There are no alarms.\n");
        return;
    }

    unsigned long index;

    // Ask for new input until valid input is given
    do {
        printf("Cancel which alarm? (Press 'x' to cancel 'cancel')\n");
        list(*alarmlist);

        char input[8];
        printf("> ");
        fgets(input, 8, stdin);
        strip_newline(input);

        if (strcmp(input, "x") == 0) {
            return;
        }

        index = strtoul(input, NULL, 10) - 1;
    } while (index >= alarmlist->size);

    // Kill the child process for the alarm
    kill(alarmlist->alarms[index].pid, SIGKILL);

    // Remove the alarm from the list.
    // This is done by overwriting the alarm to be deleted with the last alarm
    // in the list, and shrinking the list size
    alarmlist->alarms[index] = alarmlist->alarms[alarmlist->size - 1];
    alarmlist->size -= 1;
}

int main() {
    time_t current_time = time(NULL);
    alarmlist_t alarmlist = {.alarms = NULL, .size = 0};
    printf("Welcome to the alarm clock! It is currently %s\n",
           ctime(&current_time));

    char input[32];
    input[0] = '\0';
    while (strcmp(input, "x") != 0) {
        printf("\nPlease enter 's' (schedule), 'l' (list), 'c' (cancel), 'x' "
               "(exit)\n");

        printf("> ");
        fgets(input, 32, stdin);
        strip_newline(input);
        printf("\n");

        // Detect zombies and remove the alarms from the list
        for (int i = 0; i < alarmlist.size; i++) {
            if (waitpid(alarmlist.alarms[i].pid, NULL, WNOHANG) > 0) {
                alarmlist.alarms[i] = alarmlist.alarms[alarmlist.size - 1];
                alarmlist.size -= 1;
            }
        }

        if (strcmp(input, "s") == 0) {
            schedule(&alarmlist);
        } else if (strcmp(input, "l") == 0) {
            list(alarmlist);
        } else if (strcmp(input, "c") == 0) {
            cancel(&alarmlist);
        } else if (strcmp(input, "x") == 0) {
            printf("Exit\n");
        } else {
            printf("%s is not a valid command.\nPlease enter 's' "
                   "(schedule), 'l' (list), 'c' (cancel), 'x' (exit)\n",
                   input);
        }
    }

    free(alarmlist.alarms);

    return 0;
}
