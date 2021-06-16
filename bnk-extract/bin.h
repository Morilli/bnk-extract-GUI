#ifndef BIN_H
#define BIN_H

#include <inttypes.h>
#include "list.h"

struct string_hash {
    char* string;
    uint32_t hash;
    uint32_t switch_id;
};
typedef LIST(struct string_hash) StringHashes;

uint32_t fnv_1_hash(const char* input);
StringHashes* parse_bin_file(char* bin_path);

#endif
