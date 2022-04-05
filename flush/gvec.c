#include "gvec.h"
#include <stdlib.h>

#define GEN_VEC_IMPL(T, N)                                                     \
    void gvec_##N##_init(gvec_##N##_t *vec, size_t capacity) {                 \
        vec->cap = capacity;                                                   \
        vec->len = 0;                                                          \
        vec->buf = (T *)malloc(sizeof(T) * vec->cap);                          \
    }                                                                          \
                                                                               \
    void gvec_##N##_push(gvec_##N##_t *vec, T value) {                         \
        if (vec->len >= vec->cap) {                                            \
            vec->cap *= 2;                                                     \
            vec->buf = (T *)realloc(vec->buf, sizeof(T) * vec->cap);           \
        }                                                                      \
                                                                               \
        vec->buf[vec->len] = value;                                            \
        vec->len++;                                                            \
    }                                                                          \
                                                                               \
    T gvec_##N##_pop(gvec_##N##_t *vec) {                                      \
        if (vec->len == 0) {                                                   \
            return NULL;                                                       \
        }                                                                      \
                                                                               \
        vec->len--;                                                            \
        return vec->buf[vec->len];                                             \
    }                                                                          \
                                                                               \
    T gvec_##N##_get(gvec_##N##_t *vec, size_t idx) {                          \
        if (idx >= vec->len) {                                                 \
            return NULL;                                                       \
        }                                                                      \
                                                                               \
        return vec->buf[idx];                                                  \
    }                                                                          \
                                                                               \
    void gvec_##N##_destroy(gvec_##N##_t *vec) { free(vec->buf); }

// Generate generic vector implementations, specifying types and names
// NOTE: Matching declarations must be generated in the header!
GEN_VEC_IMPL(char *, str)
