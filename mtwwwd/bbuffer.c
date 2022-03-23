#include "bbuffer.h"
#include "sem.h"
#include <pthread.h>
#include <stdlib.h>

struct BNDBUF {
    // The internal buffer
    int *buffer;
    // Total buffer capacity
    size_t capacity;
    // Read index
    ptrdiff_t head;
    // Write index
    ptrdiff_t tail;
    // Counting sempahore for number of elements in the buffer
    SEM *size;
    // Counting semaphore for number of free slots in the buffer
    SEM *free;
    // Mutex to protect the buffer when reading and writing
    pthread_mutex_t mutex;
};

BNDBUF *bb_init(unsigned int capacity) {
    BNDBUF *bndbuf = malloc(sizeof(BNDBUF));
    bndbuf->buffer = malloc(capacity * sizeof(int));
    bndbuf->capacity = capacity;
    bndbuf->head = 0;
    bndbuf->tail = 0;
    bndbuf->size = sem_init(0);
    bndbuf->free = sem_init(capacity);

    if (bndbuf->size == NULL || bndbuf->free == NULL ||
        pthread_mutex_init(&bndbuf->mutex, NULL) != 0) {
        free(bndbuf->buffer);
        if (bndbuf->size != NULL)
            sem_del(bndbuf->size);
        if (bndbuf->free != NULL)
            sem_del(bndbuf->free);
        free(bndbuf);

        return NULL;
    }

    return bndbuf;
}

void bb_del(BNDBUF *bb) {
    sem_del(bb->size);
    sem_del(bb->free);
    pthread_mutex_destroy(&bb->mutex);
    free(bb->buffer);
    free(bb);
}

int bb_get(BNDBUF *bb) {
    P(bb->size);

    pthread_mutex_lock(&bb->mutex);
    int val = bb->buffer[bb->head];
    bb->head = (bb->head + 1) % bb->capacity;
    pthread_mutex_unlock(&bb->mutex);

    V(bb->free);

    return val;
}

void bb_add(BNDBUF *bb, int fd) {
    P(bb->free);

    pthread_mutex_lock(&bb->mutex);
    bb->buffer[bb->tail] = fd;
    bb->tail = (bb->tail + 1) % bb->capacity;
    pthread_mutex_unlock(&bb->mutex);

    V(bb->size);
}
