#ifndef __GVEC_H
#define __GVEC_H

#include <stdlib.h>

#define GEN_VEC_HEADER(T, N)                                                   \
    typedef struct {                                                           \
        T *buf;                                                                \
        size_t cap;                                                            \
        size_t len;                                                            \
    } gvec_##N##_t;                                                            \
                                                                               \
    void gvec_##N##_init(gvec_##N##_t *vec, size_t capacity);                  \
                                                                               \
    void gvec_##N##_push(gvec_##N##_t *vec, T value);                          \
                                                                               \
    T gvec_##N##_pop(gvec_##N##_t *vec);                                       \
                                                                               \
    T gvec_##N##_get(gvec_##N##_t *vec, size_t idx);                           \
                                                                               \
    void gvec_##N##_destroy(gvec_##N##_t *vec)

// Generate generic vector declarations, specifying types and names
// NOTE: Matching implementations must be generated in the header!
GEN_VEC_HEADER(char *, str);

#endif
