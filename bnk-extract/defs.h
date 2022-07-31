#ifndef DEFS_H
#define DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <inttypes.h>

#include "list.h"
#include "static_list.h"

extern int VERBOSE;

#ifdef _WIN32
    #define mkdir(path, mode) mkdir(path)
    #define flockfile(file) _lock_file(file)
    #define funlockfile(file) _unlock_file(file)
#endif

typedef struct {
    uint64_t length;
    uint8_t* data;
} BinaryData;

typedef struct audio_data {
    uint32_t id;
    uint32_t length;
    uint8_t* data;
} AudioData;
typedef STATIC_LIST(AudioData) AudioDataList;

typedef LIST(struct stringWithChildren) StringWithChildrenList;

typedef struct stringWithChildren {
    char* string;
    AudioData* wemData;
    StringWithChildrenList children;
} StringWithChildren;

typedef struct {
    StringWithChildren* grouped_wems;
    AudioDataList* sortedWemDataList;
} WemInformation;

#ifdef DEBUG
    #define dprintf(...) printf(__VA_ARGS__)
#else
    #define dprintf(...)
#endif
extern FILE* consoleless_stderr;
#define eprintf(...) fprintf(consoleless_stderr ? consoleless_stderr : stderr, __VA_ARGS__)
#define v_printf(level, ...) if (VERBOSE >= level) printf(__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
