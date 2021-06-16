#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>

#include "defs.h"
#include "bin.h"
#include "extract.h"
#include "api.h"

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

WemInformation* extract_bnk_file(char* bnk_path, StringHashes* string_hashes, char* output_path, bool wems_only, bool oggs_only)
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

    AudioDataList audio_data_list;
    initialize_list_size(&audio_data_list, bnkfile.length);
    for (uint32_t i = 0; i < bnkfile.length; i++) {
        add_object(&audio_data_list, (&(AudioData) {.id = bnkfile.entries[i].file_id, .data = {.length = bnkfile.entries[i].length, .data = bnkfile.entries[i].data}}));
    }

    WemInformation* grouped_wems = extract_all_audio(output_path, &audio_data_list, string_hashes, wems_only, oggs_only);

    for (uint32_t i = 0; i < bnkfile.length; i++) {
        // free(bnkfile.entries[i].data);
    }
    free(bnkfile.entries);
    free(audio_data_list.objects);

    return grouped_wems;
}
