#include "sem.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct SEM {
    unsigned int counter;
    pthread_mutex_t counter_lock;
    pthread_cond_t counter_nonzero;
};

SEM *sem_init(int initVal) {
    SEM *sem = malloc(sizeof(SEM));
    sem->counter = initVal;

    if (pthread_mutex_init(&sem->counter_lock, NULL) != 0 ||
        pthread_cond_init(&sem->counter_nonzero, NULL) != 0) {
        free(sem);
        return NULL;
    }

    return sem;
};

int sem_del(SEM *sem) {
    int status = 0;
    if (pthread_mutex_destroy(&sem->counter_lock)) {
        status = -1;
    }

    if (pthread_cond_destroy(&sem->counter_nonzero)) {
        status = -1;
    }

    free(sem);

    return status;
}

// Decrement - wait
void P(SEM *sem) {
    pthread_mutex_lock(&sem->counter_lock);

    while (sem->counter == 0) {
        pthread_cond_wait(&sem->counter_nonzero, &sem->counter_lock);
    }

    sem->counter--;
    pthread_mutex_unlock(&sem->counter_lock);
}

// Increment - signal
void V(SEM *sem) {
    pthread_mutex_lock(&sem->counter_lock);

    if (sem->counter == 0) {
        pthread_cond_signal(&sem->counter_nonzero);
    }

    sem->counter++;
    pthread_mutex_unlock(&sem->counter_lock);
}
