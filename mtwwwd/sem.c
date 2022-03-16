#include "sem.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

SEM *sem_init(int initVal) {
    SEM *sem = malloc(sizeof(SEM));
    sem->counter = initVal;
    pthread_mutex_t cl = PTHREAD_MUTEX_INITIALIZER;
    sem->counter_lock = cl;
    pthread_cond_t cn = PTHREAD_COND_INITIALIZER;
    sem->counter_nonzero = cn;

    return sem;
};

int sem_del(SEM *sem) {
    if (pthread_mutex_destroy(&sem->counter_lock)) {
        return -1;
    }

    if (pthread_cond_destroy(&sem->counter_nonzero)) {
        return -1;
    }

    free(sem);

    return 0;
}

// Decrement - wait()
void P(SEM *sem) {
    pthread_mutex_lock(&sem->counter_lock);

    while (sem->counter == 0) {
        pthread_cond_wait(&sem->counter_nonzero, &sem->counter_lock);
    }

    sem->counter--;
    pthread_mutex_unlock(&sem->counter_lock);
}

// Increment - signal()
void V(SEM *sem) {
    pthread_mutex_lock(&sem->counter_lock);

    if (sem->counter == 0) {
        pthread_cond_signal(&sem->counter_nonzero);
    }

    sem->counter++;
    pthread_mutex_unlock(&sem->counter_lock);
}
