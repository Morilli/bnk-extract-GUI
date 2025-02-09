#ifndef BIN_H
#define BIN_H

#include <stdint.h>
#include "list.h"

struct string_hash {
    char* string;
    uint32_t hash;
    uint32_t container_id;
    uint32_t music_segment_id;
    uint32_t switch_id;
    uint32_t sound_index;
};
typedef LIST(struct string_hash) StringHashes;

uint32_t fnv_1_hash(const char* input);
StringHashes* parse_bin_file(char* bin_path);

#endif
