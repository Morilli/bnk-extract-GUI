#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
char* read_string(FILE* input) {
    uint16_t string_length;
    assert(fread(&string_length, 2, 1, input) == 1);
    char* string = malloc(string_length + 1);
    assert(fread(string, 1, string_length, input) == string_length);
    string[string_length] = '\0';

    return string;
}

StringHashes* parse_bin_file(char* bin_path)
{
    FILE* bin_file = fopen(bin_path, "rb");
    if (!bin_file) {
        eprintf("Error: Failed to open \"%s\".\n", bin_path);
        return NULL;
    }

    StringHashes* saved_strings = malloc(sizeof(StringHashes));
    initialize_list(saved_strings);

    while (!feof(bin_file)) {
        if (getc(bin_file) == 0x84 && getc(bin_file) == 0xe3 && getc(bin_file) == 0xd8 && getc(bin_file) == 0x12) {
            fseek(bin_file, 6, SEEK_CUR);
            uint32_t amount;
            assert(fread(&amount, 4, 1, bin_file) == 1);
            dprintf("amount: %u\n", amount);
            for (uint32_t i = 0; i < amount; i++) {
                char* string = read_string(bin_file);
                struct string_hash new_pair = {
                    .string = string,
                    .hash = fnv_1_hash(string)
                };
                dprintf("saved string \"%s\"\n", string);
                add_object(saved_strings, &new_pair);
            }
        }
    }

    for (uint32_t i = 0; i < saved_strings->length; i++) {
        dprintf("string at position %u: \"%s\".\n", i, saved_strings->objects[i].string);
    }

    fclose(bin_file);
    return saved_strings;
}
