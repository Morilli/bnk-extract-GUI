#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "defs.h"
#include "bin.h"

uint32_t fnv_1_hash(const char* input)
{
    uint32_t hash = 0x811c9dc5;
    const char* c = input;
    while (*c != 0) {
        hash *= 0x01000193;
        hash ^= (*c > 64 && *c < 91) ? *c + 32 : *c;
        c++;
    }

    return hash;
}

// format: uint16 length, then (length) bytes string (not null-terminated).
static char* read_string(const uint8_t* buffer, int buffer_size, uint16_t* out_string_length) {
    if (buffer_size < 2) return NULL;

    uint16_t string_length = *(uint16_t*) buffer;
    if (out_string_length) *out_string_length = string_length;
    if (buffer_size < 2 + string_length) return NULL;

    char* string = malloc(string_length + 1);
    memcpy(string, buffer + 2, string_length);
    string[string_length] = '\0';

    return string;
}

#ifndef _DEFAULT_SOURCE
static uint8_t* memmem(const uint8_t* haystack, size_t haystackLen, const uint8_t* needle, size_t needleLen)
{
    if (needleLen > haystackLen) return NULL;

    const uint8_t* max_start = haystack + haystackLen - needleLen;

    for (const uint8_t* current = haystack; current <= max_start; current++) {
        if (memcmp(current, needle, needleLen) == 0) {
            return (uint8_t*) current;
        }
    }

    return NULL;
}
#endif

StringHashes* parse_bin_file(char* bin_path)
{
    FILE* bin_file = fopen(bin_path, "rb");
    if (!bin_file) {
        eprintf("Error: Failed to open \"%s\".\n", bin_path);
        return NULL;
    }

    StringHashes* saved_strings = malloc(sizeof(StringHashes));
    initialize_list(saved_strings);

    fseek(bin_file, 0, SEEK_END);
    size_t bin_size = ftell(bin_file);
    rewind(bin_file);

    uint8_t* buffer = malloc(bin_size);
    assert(fread(buffer, 1, bin_size, bin_file) == bin_size);
    fclose(bin_file);

#define BUFFER_REMAINING (buffer + bin_size - current_position)

    // search for 'events' containers
    {
    uint8_t to_search[] = { 0x84, 0xE3, 0xD8, 0x12, 0x80, 0x10 };
    uint8_t* current_position = buffer;
    while (1) {
        uint8_t* found = memmem(current_position, BUFFER_REMAINING, to_search, sizeof(to_search));
        if (!found) break;

        current_position = found + sizeof(to_search);

        if (BUFFER_REMAINING < 8) goto error;
        current_position += 4; // skip object size
        uint32_t amount = *(uint32_t*) current_position;
        current_position += 4;
        dprintf("event amount: %u\n", amount);

        for (uint32_t i = 0; i < amount; i++) {
            uint16_t string_length = 0;
            char* string = read_string(current_position, BUFFER_REMAINING, &string_length);
            if (!string) goto error;
            struct string_hash new_pair = {
                .string = string,
                .hash = fnv_1_hash(string)
            };
            dprintf("saved string \"%s\"\n", string);
            add_object(saved_strings, &new_pair);
            current_position += 2 + string_length;
        }
    }}

    // search for 'music' embedded objects
    {
    uint8_t to_search[] = { 0xD4, 0x4F, 0x9C, 0x9F, 0x83 };
    uint8_t* current_position = buffer;
    while (1) {
        uint8_t* found = memmem(current_position, BUFFER_REMAINING, to_search, sizeof(to_search));
        if (!found) break;

        current_position = found + sizeof(to_search);

        if (BUFFER_REMAINING < 4) goto error;
        uint32_t type_hash = *(uint32_t*) current_position;
        current_position += 4;
        if (!type_hash) {
            continue;
        }

        if (BUFFER_REMAINING < 6) goto error;
        current_position += 4; // skip object size
        uint16_t amount = *(uint16_t*) current_position;
        current_position += 2;
        dprintf("music amount: %u\n", amount);

        for (uint16_t i = 0; i < amount; i++) {
            if (BUFFER_REMAINING < 5) goto error;
            current_position += 4; // skip name (hash)
            uint8_t bin_type = *current_position;
            if (bin_type != 0x10 /* string */) goto error;
            current_position++;

            uint16_t string_length = 0;
            char* string = read_string(current_position, BUFFER_REMAINING, &string_length);
            if (!string) goto error;
            struct string_hash new_pair = {
                .string = string,
                .hash = fnv_1_hash(string)
            };
            dprintf("saved string \"%s\"\n", string);
            add_object(saved_strings, &new_pair);
            current_position += 2 + string_length;
        }
    }}

    free(buffer);

    for (uint32_t i = 0; i < saved_strings->length; i++) {
        dprintf("string at position %u: \"%s\".\n", i, saved_strings->objects[i].string);
    }

    return saved_strings;

    error:
    free(buffer);
    free(saved_strings->objects);
    free(saved_strings);
    eprintf("Error: Malformed/truncated bin file!\n");
    return NULL;
}
