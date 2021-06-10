#ifndef UTLITY_H
#define UTILITY_H

#include <vorbis/vorbisfile.h>
#include <stdint.h>

extern HINSTANCE me;
extern int worker_thread_pipe[2];

typedef struct {
    const uint8_t* data;
    uint64_t position;
    uint64_t size;
} ReadableBinaryData;

extern ov_callbacks oggCallbacks;

uint8_t* WavFromOgg(ReadableBinaryData* oggData);

void* PlayAudio(void* _args);

size_t read_func_callback(void* ptr, size_t size, size_t nmemb, void* datasource);
int seek_func_callback(void* datasource, ogg_int64_t offset, int whence);
long tell_func_callback(void* datasource);

#endif
