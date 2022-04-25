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
#include <vorbis/codec.h>

BinaryData* WemToOgg(AudioData* wemData)
{
    char data_pointer[17] = {0};
    bytes2hex(&wemData, data_pointer, 8);
    char* ww2ogg_args[] = {"", "--audiodata", data_pointer, NULL};
    vorbis_info_clear(NULL); // NECESSARY FOR SOME REASON
    BinaryData* raw_ogg = ww2ogg(sizeof(ww2ogg_args) / sizeof(ww2ogg_args[0]) - 1, ww2ogg_args);
    // if (!raw_ogg)
        return NULL;

    // if (memcmp(raw_ogg->data, "RIFF", 4) == 0) { // got a wav file instead of an ogg one
        // return raw_ogg;
    // } else {
        // bytes2hex(&raw_ogg, data_pointer, 8);
        // const char* revorb_args[] = {"", data_pointer, NULL};
        // BinaryData* converted_ogg_data = revorb(sizeof(revorb_args) / sizeof(revorb_args[0]) - 1, revorb_args);
        // free(raw_ogg->data);
        // free(raw_ogg);

        // return converted_ogg_data;
    // }
}
