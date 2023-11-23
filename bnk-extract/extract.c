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

static StringWithChildrenList* try_insert(StringWithChildrenList* root, char* name)
{
    bool found = false;
    uint32_t i;
    for (i = 0; i < root->length; i++) {
        if ((strcmp(root->objects[i].string, name) == 0)) {
            found = true;
            break;
        }
    }

    if (!found) {
        StringWithChildren newObject = {.string = strdup(name)};
        initialize_list(&newObject.children);
        add_object(root, &newObject);
    }

    StringWithChildrenList* inserted_element = &root->objects[i].children;

    return inserted_element;
}

StringWithChildren* group_wems(AudioDataList* audio_data, StringHashes* string_hashes)
{
    StringWithChildren* grouped_wems = calloc(1, sizeof(StringWithChildren));
    initialize_list(&grouped_wems->children);

    for (uint32_t i = 0; i < audio_data->length; i++) {
        AudioData* current_audio_data = &audio_data->objects[i];
        bool inserted = false;
        for (uint32_t string_index = 0; string_index < string_hashes->length; string_index++) {
            if (string_hashes->objects[string_index].hash == current_audio_data->id) {
                StringWithChildrenList* current_root = &grouped_wems->children;
                struct string_hash* current_event = &string_hashes->objects[string_index];

                if (current_event->switch_id) {
                    char switch_id[11];
                    sprintf(switch_id, "%u", current_event->switch_id);
                    current_root = try_insert(current_root, switch_id);
                }
                current_root = try_insert(current_root, current_event->string);

                char wem_name[15];
                sprintf(wem_name, "%u.wem", current_audio_data->id);
                add_object(current_root, (&(StringWithChildren) {.string = strdup(wem_name), .wemData = current_audio_data}));
                inserted = true;
            }
        }
        if (!inserted) {
            char wem_name[15];
            sprintf(wem_name, "%u.wem", current_audio_data->id);
            add_object(&grouped_wems->children, (&(StringWithChildren) {.string = strdup(wem_name), .wemData = current_audio_data}));
        }
    }

    return grouped_wems;
}
