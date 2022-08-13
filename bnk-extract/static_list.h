#ifndef STATIC_LIST_H
#define STATIC_LIST_H

#include <stdint.h>
#include <stdlib.h>

#define STATIC_LIST(type) struct { \
    uint64_t length; \
    type* objects; \
}

#define initialize_static_list(list, size) do { \
    (list)->length = size; \
    (list)->objects = calloc(size, sizeof((list)->objects[0])); \
} while (0)

#define sort_static_list(list, key) do { \
    int n = (list)->length; \
    __typeof__(list) temp = malloc(sizeof(__typeof__(*list))); \
    temp->length = (list)->length; \
    temp->objects = malloc((list)->length * sizeof(__typeof__((list)->objects[0]))); \
    memcpy(temp->objects, (list)->objects, (list)->length * sizeof(__typeof__((list)->objects[0]))); \
    int parity = 0; \
    for (int width = 1; width < n; width = 2 * width) { \
        parity++; \
        for (int i = 0; i < n; i = i + 2 * width) { \
            if (parity % 2 == 0) \
                sortHelper(list, i, min(i+width, n), min(i+2*width, n), temp, key); \
            else \
                sortHelper(temp, i, min(i+width, n), min(i+2*width, n), list, key); \
        } \
    } \
    if (parity % 2 == 0) { \
        free((list)->objects); \
        (list)->objects = temp->objects; \
        free(temp); \
    } else { \
        free(temp->objects); \
        free(temp); \
    } \
} while (0)

#define sortHelper(listLeft, iLeft, iRight, iEnd, listRight, key) do { \
    int ___i = iLeft, ___j = iRight; \
    for (int ___k = iLeft; ___k < iEnd; ___k++) { \
        if (___i < iRight && (___j >= iEnd || (listLeft)->objects[___i].key <= (listLeft)->objects[___j].key)) { \
            (listRight)->objects[___k] = (listLeft)->objects[___i]; \
            ___i++; \
        } else { \
            (listRight)->objects[___k] = (listLeft)->objects[___j]; \
            ___j++; \
        } \
    } \
} while (0)

#endif
