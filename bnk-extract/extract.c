#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bin.h"
#include "defs.h"
#include "general_utils.h"
#include "ww2ogg/api.h"
#include "revorb/api.h"

BinaryData* WemToOgg(AudioData* wemData)
{
    char data_pointer[17] = {0};
    bytes2hex(&wemData, data_pointer, 8);
    char* ww2ogg_args[] = {"", "--audiodata", data_pointer, NULL};
    BinaryData* raw_ogg = ww2ogg(sizeof(ww2ogg_args) / sizeof(ww2ogg_args[0]) - 1, ww2ogg_args);
    if (!raw_ogg)
        return NULL;

    if (memcmp(raw_ogg->data, "RIFF", 4) == 0) { // got a wav file instead of an ogg one
        return raw_ogg;
    } else {
        bytes2hex(&raw_ogg, data_pointer, 8);
        const char* revorb_args[] = {"", data_pointer, NULL};
        BinaryData* converted_ogg_data = revorb(sizeof(revorb_args) / sizeof(revorb_args[0]) - 1, revorb_args);
        free(raw_ogg->data);
        free(raw_ogg);

        return converted_ogg_data;
    }
}

StringWithChildren* group_wems(AudioDataList* audio_data, StringHashes* string_hashes)
{
    StringWithChildren* grouped_wems = calloc(1, sizeof(StringWithChildren));
    initialize_list(&grouped_wems->children);

    for (uint32_t i = 0; i < audio_data->length; i++) {
        bool inserted = false;
        for (uint32_t string_index = 0; string_index < string_hashes->length; string_index++) {
            if (string_hashes->objects[string_index].hash == audio_data->objects[i].id) {
                StringWithChildrenList* current_root = &grouped_wems->children;
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
                if (do_again && string_hashes->objects[string_index].switch_id) {
                    do_again = false;
                    current_root = &grouped_wems->children.objects[j].children;
                    goto find_object;
                }
                char wem_name[15];
                sprintf(wem_name, "%u.wem", audio_data->objects[i].id);
                add_object(&current_root->objects[j].children, (&(StringWithChildren) {.string = strdup(wem_name), .wemData = &audio_data->objects[i]}));
                inserted = true;
            }
        }
        if (!inserted) {
            char wem_name[15];
            sprintf(wem_name, "%u.wem", audio_data->objects[i].id);
            add_object(&grouped_wems->children, (&(StringWithChildren) {.string = strdup(wem_name), .wemData = &audio_data->objects[i]}));
        }
    }

    return grouped_wems;
}
