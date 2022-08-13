#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "defs.h"
#include "bin.h"
#include "extract.h"
#include "static_list.h"

struct BNKFileEntry {
    uint32_t file_id;
    uint32_t offset;
    uint32_t length;
    uint8_t* data;
};

struct BNKFile {
    uint32_t length;
    struct BNKFileEntry* entries;
};

uint32_t skip_to_section(FILE* bnk_file, char name[4], bool from_beginning)
{
    if (from_beginning)
        fseek(bnk_file, 0, SEEK_SET);

    uint8_t header[4];
    uint32_t section_length = 0;

    do {
        fseek(bnk_file, section_length, SEEK_CUR);
        if (fread(header, 1, 4, bnk_file) != 4)
            return 0;
        assert(fread(&section_length, 4, 1, bnk_file) == 1);
    } while (memcmp(header, name, 4) != 0);

    return section_length;
}

int parse_bnk_file_entries(FILE* bnk_file, struct BNKFile* bnkfile)
{
    uint32_t section_length = skip_to_section(bnk_file, "DIDX", false);
    if (!section_length)
        return -1;

    bnkfile->length = section_length / 12;
    bnkfile->entries = malloc(bnkfile->length * sizeof(struct BNKFileEntry));
    for (uint32_t i = 0; i < bnkfile->length; i++) {
        assert(fread(&bnkfile->entries[i].file_id, 4, 1, bnk_file) == 1);
        assert(fread(&bnkfile->entries[i].offset, 4, 1, bnk_file) == 1);
        assert(fread(&bnkfile->entries[i].length, 4, 1, bnk_file) == 1);
    }

    section_length = skip_to_section(bnk_file, "DATA", false);
    if (!section_length)
        return -1;

    uint32_t data_offset = ftell(bnk_file);
    for (uint32_t i = 0; i < bnkfile->length; i++) {
        fseek(bnk_file, data_offset + bnkfile->entries[i].offset, SEEK_SET);
        bnkfile->entries[i].data = malloc(bnkfile->entries[i].length);
        assert(fread(bnkfile->entries[i].data, 1, bnkfile->entries[i].length, bnk_file) == bnkfile->entries[i].length);
    }

    return 0;
}

WemInformation* parse_audio_bnk_file(char* bnk_path, StringHashes* string_hashes)
{
    FILE* bnk_file = fopen(bnk_path, "rb");
    if (!bnk_file) {
        eprintf("Error: Failed to open \"%s\".\n", bnk_path);
        return NULL;
    }

    struct BNKFile bnkfile;
    if (parse_bnk_file_entries(bnk_file, &bnkfile) == -1) {
        eprintf("Error: Failed to find the required sections in file \"%s\". Make sure to provide the correct file.\n", bnk_path);
        return NULL;
    }
    fclose(bnk_file);

    WemInformation* wem_information = malloc(sizeof(WemInformation));
    wem_information->sortedWemDataList = malloc(sizeof(AudioDataList));
    initialize_static_list(wem_information->sortedWemDataList, bnkfile.length);
    for (uint32_t i = 0; i < bnkfile.length; i++) {
        wem_information->sortedWemDataList->objects[i] = (AudioData) {
            .id = bnkfile.entries[i].file_id,
            .length = bnkfile.entries[i].length,
            .data = bnkfile.entries[i].data
        };
    }
    sort_static_list(wem_information->sortedWemDataList, id);

    free(bnkfile.entries);

    wem_information->grouped_wems = group_wems(wem_information->sortedWemDataList, string_hashes);

    return wem_information;
}
