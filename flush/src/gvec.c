// A generic vector
//
// The GEN_VEC_IMPL macro generates function definitions for creating and
// modifying type-safe vectors.

#include "gvec.h"
#include <string.h>

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
            return (T){0};                                                     \
        }                                                                      \
                                                                               \
        vec->len--;                                                            \
        return vec->buf[vec->len];                                             \
    }                                                                          \
                                                                               \
    void gvec_##N##_remove(gvec_##N##_t *vec, size_t idx) {                    \
        if (vec->len == 0 || idx >= vec->len) {                                \
            return;                                                            \
        }                                                                      \
                                                                               \
        memmove(vec->buf + idx, vec->buf + idx + 1,                            \
                sizeof(T) * (vec->len - idx - 1));                             \
        vec->len--;                                                            \
    }                                                                          \
                                                                               \
    void gvec_##N##_destroy(gvec_##N##_t *vec) { free(vec->buf); }

// Generate generic vector implementations, specifying types and names
// NOTE: Matching declarations must be generated in the header!
GEN_VEC_IMPL(char *, str)
GEN_VEC_IMPL(char, char)
GEN_VEC_IMPL(int, int)
