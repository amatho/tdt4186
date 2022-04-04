#ifndef __GPVEC_H
#define __GPVEC_H

#include <stddef.h>

typedef struct {
    void **buf;
    size_t cap;
    size_t len;
} gpvec_t;

void gpvec_init(gpvec_t *vec, size_t capacity);

void gpvec_push(gpvec_t *vec, void *value);

void *gpvec_pop(gpvec_t *vec);

void *gpvec_get(gpvec_t *vec, size_t idx);

void gpvec_destroy(gpvec_t *vec);

#endif
