#include "gpvec.h"
#include <stdio.h>
#include <stdlib.h>

void gpvec_init(gpvec_t *vec, size_t capacity) {
    vec->cap = capacity;
    vec->len = 0;
    vec->buf = malloc(sizeof(void *) * vec->cap);
}

void gpvec_push(gpvec_t *vec, void *value) {
    if (vec->len >= vec->cap) {
        vec->cap *= 2;
        vec->buf = realloc(vec->buf, sizeof(void *) * vec->cap);
    }

    vec->buf[vec->len] = value;
    vec->len++;
}

void *gpvec_pop(gpvec_t *vec) {
    if (vec->len == 0) {
        return NULL;
    }

    vec->len--;
    return vec->buf[vec->len];
}

void *gpvec_get(gpvec_t *vec, size_t idx) {
    if (idx >= vec->len) {
        return NULL;
    }

    return vec->buf[idx];
}

void gpvec_destroy(gpvec_t *vec) { free(vec->buf); }
