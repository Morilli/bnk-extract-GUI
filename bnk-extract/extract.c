#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#   include <windows.h>
#else
#   include <unistd.h>
#endif

#include "bin.h"
#include "extract.h"
#include "defs.h"
#include "general_utils.h"
#include "ww2ogg/api.h"
#include "revorb/api.h"
#include "api.h"

BinaryData* WemToOgg(BinaryData* wemData)
{
    char data_pointer[17] = {0};
    bytes2hex(&wemData, data_pointer, 8);
    char* ww2ogg_args[] = {"", "--binarydata", data_pointer, NULL};
    BinaryData* raw_ogg = ww2ogg(sizeof(ww2ogg_args) / sizeof(ww2ogg_args[0]) - 1, ww2ogg_args);
    if (!raw_ogg)
        return NULL;
    if (memcmp(raw_ogg->data, "RIFF", 4) == 0) { // got a wav file instead of an ogg one
        // memcpy(&ogg_path[string_length - 5], ".wav", 5);
        // FILE* wav_file = fopen(ogg_path, "wb");
        // if (!wav_file) {
            // eprintf("Could not open output file.\n");
            // goto end;
        // }
        // fwrite(raw_ogg->data, raw_ogg->length, 1, wav_file);
        // fclose(wav_file);
        // type |= 4;
        return raw_ogg;
    } else {
        // memcpy(&ogg_path[string_length - 5], ".ogg", 5);
        bytes2hex(&raw_ogg, data_pointer, 8);
        const char* revorb_args[] = {"", data_pointer, NULL};
        BinaryData* converted_ogg_data = revorb(sizeof(revorb_args) / sizeof(revorb_args[0]) - 1, revorb_args);
        free(raw_ogg->data);
        free(raw_ogg);

        return converted_ogg_data;
            // goto end;
        // }
        // type |= 2;
    }
    // end:
}

void hardlink_file(char* existing_path, char* new_path, uint8_t type)
{
    if (type & 1) {
        if (link(existing_path, new_path) != 0 && errno != EEXIST) {
            eprintf("Error: Failed hard-linking \"%s\" to \"%s\"\n", new_path, existing_path);
            return;
        }
    }
    if (type <= 1) return;

    int existing_path_length = strlen(existing_path);
    int new_path_length = strlen(new_path);
    char converted_existing_path[existing_path_length+1], converted_new_path[new_path_length+1];
    memcpy(converted_existing_path, existing_path, existing_path_length);
    memcpy(converted_existing_path + existing_path_length - 4, type & 2 ? ".ogg" : ".wav", 5);
    memcpy(converted_new_path, new_path, new_path_length);
    memcpy(converted_new_path + new_path_length - 4, type & 2 ? ".ogg" : ".wav", 5);
    if (link(converted_existing_path, converted_new_path) != 0 && errno != EEXIST) {
        eprintf("Error: Failed hard-linking \"%s\" to \"%s\"\n", converted_new_path, converted_existing_path);
        return;
    }
}

uint8_t extract_audio(char* output_path, BinaryData* wem_data, bool wem_only, bool ogg_only)
{
    uint8_t type = 0;
    if (!ogg_only) {
        FILE* output_file = fopen(output_path, "wb");
        if (!output_file) {
            eprintf("Error: Failed to open \"%s\"\n", output_path);
            return type;
        }
        v_printf(1, "Extracting \"%s\"\n", output_path);
        fwrite(wem_data->data, 1, wem_data->length, output_file);
        fclose(output_file);
        type |= 1;
    }

    if (!wem_only) {
        // convert the .wem to .ogg
        int string_length = strlen(output_path) + 1;
        char ogg_path[string_length];
        memcpy(ogg_path, output_path, string_length);
        ogg_path[string_length - 5] = '\0';

        v_printf(1, "Extracting \"%s", ogg_path);
        fflush(stdout);
        char data_pointer[17] = {0};
        bytes2hex(&wem_data, data_pointer, 8);
        char* ww2ogg_args[] = {"", "--binarydata", data_pointer, NULL};
        BinaryData* raw_ogg = ww2ogg(sizeof(ww2ogg_args) / sizeof(ww2ogg_args[0]) - 1, ww2ogg_args);
        if (!raw_ogg)
            return type;
        if (memcmp(raw_ogg->data, "RIFF", 4) == 0) { // got a wav file instead of an ogg one
            memcpy(&ogg_path[string_length - 5], ".wav", 5);
            FILE* wav_file = fopen(ogg_path, "wb");
            if (!wav_file) {
                eprintf("Could not open output file.\n");
                goto end;
            }
            fwrite(raw_ogg->data, raw_ogg->length, 1, wav_file);
            fclose(wav_file);
            type |= 4;
        } else {
            memcpy(&ogg_path[string_length - 5], ".ogg", 5);
            bytes2hex(&raw_ogg, data_pointer, 8);
            const char* revorb_args[] = {"", data_pointer, ogg_path, NULL};
            if (revorb(sizeof(revorb_args) / sizeof(revorb_args[0]) - 1, revorb_args) != 0) {
                goto end;
            }
            type |= 2;
        }
        end:
        free(raw_ogg->data);
        free(raw_ogg);
    }

    return type;
}

WemInformation* extract_all_audio(char* output_path, AudioDataList* audio_data, StringHashes* string_hashes, bool wems_only, bool oggs_only)
{
    WemInformation* wem_information = malloc(sizeof(WemInformation));
    wem_information->sortedWemDataList = malloc(sizeof(IndexedDataList));
    initialize_list_size(wem_information->sortedWemDataList, audio_data->length);
    for (uint32_t i = 0; i < audio_data->length; i++) {
        IndexedData newElement = {
            .id = audio_data->objects[i].id,
            .wemData = malloc(sizeof(BinaryData))
        };
        *newElement.wemData = audio_data->objects[i].data;
        add_object(wem_information->sortedWemDataList, &newElement);
    }
    wem_information->grouped_wems = calloc(1, sizeof(StringWithChildren));
    initialize_list(&wem_information->grouped_wems->children);
    // create_dirs(output_path, true);

    for (uint32_t i = 0; i < audio_data->length; i++) {
        bool extracted = false;
        for (uint32_t string_index = 0; string_index < string_hashes->length; string_index++) {
            if (string_hashes->objects[string_index].hash == audio_data->objects[i].id) {
                StringWithChildrenList* current_root = &wem_information->grouped_wems->children;
                bool do_again = true;
                find_object:;
                bool found = false;
                uint32_t j;
                char switch_id[11];
                if (!do_again) {
                    sprintf(switch_id, "%u", string_hashes->objects[string_index].switch_id);
                }
                for (j = 0; j < current_root->length; j++) {
                    if ((!do_again && strcmp(current_root->objects[j].string, switch_id) == 0) ||
                        (strcmp(current_root->objects[j].string, string_hashes->objects[string_index].string) == 0)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    StringWithChildren newObject = {.string = do_again ? strdup(string_hashes->objects[string_index].string) : strdup(switch_id)};
                    initialize_list(&newObject.children);
                    add_object(current_root, &newObject);
                }
                if (string_hashes->objects[string_index].switch_id && do_again) {
                    do_again = false;
                    current_root = &wem_information->grouped_wems->children.objects[j].children;
                    goto find_object;
                }
                char wem_name[15];
                sprintf(wem_name, "%u.wem", audio_data->objects[i].id);
                add_object(&current_root->objects[j].children, (&(StringWithChildren) {.string = strdup(wem_name), .wemData = wem_information->sortedWemDataList->objects[i].wemData}));
                extracted = true;
            }
        }
        if (!extracted) {
            // char cur_output_path[strlen(output_path) + 16];
            // sprintf(cur_output_path, "%s/%u.wem", output_path, audio_data->objects[i].id);
            char wem_name[15];
            sprintf(wem_name, "%u.wem", audio_data->objects[i].id);
            add_object(&wem_information->grouped_wems->children, (&(StringWithChildren) {.string = strdup(wem_name), .wemData = wem_information->sortedWemDataList->objects[i].wemData}));
            // extract_audio(cur_output_path, &audio_data->objects[i].data, wems_only, oggs_only);
        // } else {
            // free(initial_output_path);
        }
    }

    return wem_information;
}
