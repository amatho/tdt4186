#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    time_t time;
    pid_t pid;
} alarm_t;

typedef struct {
    alarm_t *alarms;
    uintptr_t size;
} alarmlist_t;

void strip_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

void schedule(alarmlist_t *alarmlist) {
    char input[32];
    struct tm date;

    do {
        printf("Schedule alarm at which date and time? ");
        fgets(input, 32, stdin);
        strip_newline(input);
    } while (strptime(input, "%Y-%m-%d %H:%M:%S", &date) == NULL);

    time_t alarm_time = mktime(&date);

    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Could not spawn a child process");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        unsigned int df = (unsigned int)difftime(alarm_time, time(NULL));
        sleep(df);

#if __APPLE__
        char *mp_player = "afplay";
#elif __linux__
        char *mp_player = "mpg123";
#else
#error "Unsupported system"
#endif

        execlp(mp_player, mp_player, "-t", "20", "ringtone.mp3", NULL);

        _exit(EXIT_FAILURE);
    } else {
        alarm_t alarm = {.time = alarm_time, .pid = pid};
        alarmlist->size += 1;
        alarmlist->alarms =
            realloc(alarmlist->alarms, sizeof(alarm_t) * (alarmlist->size));
        alarmlist->alarms[alarmlist->size - 1] = alarm;
    }
}

void list(alarmlist_t alarmlist) {
    printf("Alarms:\n");
    for (int i = 0; i < alarmlist.size; i++) {
        alarm_t a = alarmlist.alarms[i];
        printf("- Alarm %d: %s", i + 1, ctime(&a.time));
    }
}

void cancel(alarmlist_t *alarmlist) {
    if (alarmlist->size == 0) {
        printf("There are no alarms.\n");
        return;
    }

    unsigned long index;
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

    kill(alarmlist->alarms[index].pid, SIGKILL);

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