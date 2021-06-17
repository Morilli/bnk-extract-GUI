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

#endif
