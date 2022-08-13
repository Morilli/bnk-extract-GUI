#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>

#include "defs.h"
#include "bin.h"
#include "extract.h"
#include "static_list.h"

struct WPKFileEntry {
    uint32_t data_offset;
    uint32_t data_length;
    char* filename;
    uint8_t* data;
};

struct WPKFile {
    uint8_t magic[4];
    uint32_t version;
    uint32_t file_count;
    uint32_t offset_amount;
    uint32_t* offsets;
    struct WPKFileEntry* wpk_file_entries;
};


void parse_header(FILE* wpk_file, struct WPKFile* wpkfile)
{
    assert(fread(wpkfile->magic, 1, 4, wpk_file) == 4);
    assert(memcmp(wpkfile->magic, "r3d2", 4) == 0);
    assert(fread(&wpkfile->version, 4, 1, wpk_file) == 1);
    assert(fread(&wpkfile->file_count, 4, 1, wpk_file) == 1);
}

void parse_offsets(FILE* wpk_file, struct WPKFile* wpkfile)
{
    wpkfile->offsets = malloc(wpkfile->file_count * 4);
    wpkfile->offset_amount = wpkfile->file_count;
    assert(fread(wpkfile->offsets, 4, wpkfile->file_count, wpk_file) == wpkfile->file_count);
}

void parse_data(FILE* wpk_file, struct WPKFile* wpkfile)
{
    wpkfile->wpk_file_entries = malloc(wpkfile->file_count * sizeof(struct WPKFileEntry));
    for (uint32_t i = 0; i < wpkfile->offset_amount; i++) {
        fseek(wpk_file, wpkfile->offsets[i], SEEK_SET);

        assert(fread(&wpkfile->wpk_file_entries[i].data_offset, 4, 1, wpk_file) == 1);
        assert(fread(&wpkfile->wpk_file_entries[i].data_length, 4, 1, wpk_file) == 1);

        int filename_size;
        assert(fread(&filename_size, 4, 1, wpk_file) == 1);
        wpkfile->wpk_file_entries[i].filename = malloc(filename_size + 1);
        for (int j = 0; j < filename_size; j++) {
            wpkfile->wpk_file_entries[i].filename[j] = getc(wpk_file);
            fseek(wpk_file, 1, SEEK_CUR);
        }
        wpkfile->wpk_file_entries[i].filename[filename_size] = '\0';
        dprintf("string: \"%s\"\n", wpkfile->wpk_file_entries[i].filename);

        wpkfile->wpk_file_entries[i].data = malloc(wpkfile->wpk_file_entries[i].data_length);
        fseek(wpk_file, wpkfile->wpk_file_entries[i].data_offset, SEEK_SET);
        assert(fread(wpkfile->wpk_file_entries[i].data, 1, wpkfile->wpk_file_entries[i].data_length, wpk_file) == wpkfile->wpk_file_entries[i].data_length);
    }
}

WemInformation* parse_wpk_file(char* wpk_path, StringHashes* string_hashes)
{
    FILE* wpk_file = fopen(wpk_path, "rb");
    if (!wpk_file) {
        eprintf("Error: Failed to open \"%s\".\n", wpk_path);
        return NULL;
    }

    struct WPKFile wpkfile;
    parse_header(wpk_file, &wpkfile);
    parse_offsets(wpk_file, &wpkfile);
    parse_data(wpk_file, &wpkfile);
    fclose(wpk_file);

    WemInformation* wem_information = malloc(sizeof(WemInformation));
    wem_information->sortedWemDataList = malloc(sizeof(AudioDataList));
    initialize_static_list(wem_information->sortedWemDataList, wpkfile.file_count);
    for (uint32_t i = 0; i < wpkfile.file_count; i++) {
        wem_information->sortedWemDataList->objects[i] = (AudioData) {
            .id = strtoul(wpkfile.wpk_file_entries[i].filename, NULL, 10),
            .length = wpkfile.wpk_file_entries[i].data_length,
            .data = wpkfile.wpk_file_entries[i].data
        };
    }
    sort_static_list(wem_information->sortedWemDataList, id);

    for (uint32_t i = 0; i < wpkfile.file_count; i++) {
        free(wpkfile.wpk_file_entries[i].filename);
    }
    free(wpkfile.offsets);
    free(wpkfile.wpk_file_entries);

    wem_information->grouped_wems = group_wems(wem_information->sortedWemDataList, string_hashes);

    return wem_information;
}
