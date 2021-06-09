#include <stdlib.h>
#include <string.h>
#include <vorbis/vorbisfile.h>

#include "utility.h"

uint8_t* WavFromOgg(ReadableBinaryData* oggData)
{
    OggVorbis_File oggDataInfo;
    // exampleOgg.position = 0;
    int return_value = ov_open_callbacks(oggData, &oggDataInfo, NULL, 0, oggCallbacks);
    printf("return value: %d\n", return_value);
    if (return_value != 0) return NULL;
    size_t rawPcmSize = ov_pcm_total(&oggDataInfo, -1) * oggDataInfo.vi->channels * 2; // 16 bit?
    uint8_t* rawPcmDataFromOgg = malloc(rawPcmSize + 44);
    memcpy(rawPcmDataFromOgg, "RIFF", 4);
    memcpy(rawPcmDataFromOgg + 4, &(uint32_t) {rawPcmSize + 36}, 4);
    memcpy(rawPcmDataFromOgg + 8, "WAVEfmt ", 8);
    memcpy(rawPcmDataFromOgg + 16, &(uint32_t) {0x10}, 4);
    memcpy(rawPcmDataFromOgg + 20, &(uint32_t) {0x01}, 2);
    memcpy(rawPcmDataFromOgg + 22, &oggDataInfo.vi->channels, 2);
    memcpy(rawPcmDataFromOgg + 24, &oggDataInfo.vi->rate, 4);
    memcpy(rawPcmDataFromOgg + 28, &(uint32_t) {oggDataInfo.vi->rate * oggDataInfo.vi->channels * 2}, 4);
    memcpy(rawPcmDataFromOgg + 32, &(uint32_t) {oggDataInfo.vi->channels * 2}, 2);
    memcpy(rawPcmDataFromOgg + 34, &(uint32_t) {16}, 2);
    memcpy(rawPcmDataFromOgg + 36, "data", 4);
    memcpy(rawPcmDataFromOgg + 40, &rawPcmSize, 4);
    size_t current_position = 0;
    while (current_position < rawPcmSize) {
        current_position += ov_read(&oggDataInfo, (char*) rawPcmDataFromOgg + 44 + current_position, rawPcmSize, 0, 2, 1, &(int) {0});
    }
    ov_clear(&oggDataInfo);
    // printf("return value: %d\n", PlaySound((LPCSTR) rawPcmDataFromOgg, me, SND_MEMORY));
    // free(rawPcmDataFromOgg);

    return rawPcmDataFromOgg;
}

size_t read_func_callback(void* ptr, size_t size, size_t nmemb, void* datasource)
{
    ReadableBinaryData* source = datasource;
    size_t total_bytes = size * nmemb;

    if (source->size < source->position + total_bytes) {
        total_bytes = source->size - source->position;
    }

    memcpy(ptr, source->data + source->position, total_bytes);

    source->position += total_bytes;
    return total_bytes;
}

int seek_func_callback(void* datasource, ogg_int64_t offset, int whence)
{
    ReadableBinaryData* source = datasource;

    ssize_t requested_position = offset;
    if (whence == SEEK_CUR) requested_position += source->position;
    if (whence == SEEK_END) requested_position += source->size;
    if ((size_t) requested_position > source->size || requested_position < 0) return -1;

    source->position = requested_position;
    return 0;
}

long tell_func_callback(void* datasource)
{
    ReadableBinaryData* source = datasource;

    return source->position;
}

ov_callbacks oggCallbacks = {
    .read_func = read_func_callback,
    .seek_func = seek_func_callback,
    .tell_func = tell_func_callback,
    .close_func = NULL
};
