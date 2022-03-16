#include "bbuffer.h"
#include "sem.h"
#include <stdlib.h>

BNDBUF *bb_init(unsigned int capacity) {
    BNDBUF *bndbuf = malloc(sizeof(BNDBUF));
    bndbuf->buffer = malloc(capacity * sizeof(int));
    bndbuf->capacity = capacity;
    // read pointer
    bndbuf->head = 0;
    // write pointer
    bndbuf->tail = 0;
    bndbuf->size = sem_init(0);
    bndbuf->free = sem_init(capacity);
    pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    bndbuf->mutex = m;

    return bndbuf;
}

void bb_del(BNDBUF *bb) {
    sem_del(bb->size);
    sem_del(bb->free);
    free(bb->buffer);
    free(bb);
}

int bb_get(BNDBUF *bb) {
    P(bb->size);
    V(bb->free);

    pthread_mutex_lock(&bb->mutex);
    int val = bb->buffer[bb->head];
    bb->head = (bb->head + 1) % bb->capacity;
    pthread_mutex_unlock(&bb->mutex);

    return val;
}

void bb_add(BNDBUF *bb, int fd) {
    P(bb->free);
    V(bb->size);

    pthread_mutex_lock(&bb->mutex);
    bb->buffer[bb->tail] = fd;
    bb->tail = (bb->tail + 1) % bb->capacity;
    pthread_mutex_unlock(&bb->mutex);
}
