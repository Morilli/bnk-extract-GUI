#ifndef UTILITY_H
#define UTILITY_H

#include <vorbis/vorbisfile.h>
#include <stdint.h>
#include <dwmapi.h>
#include "list.h"

typedef struct {
    uint32_t id;
    BinaryData* wemData;
} IndexedData;
typedef LIST(IndexedData) IndexedDataList;

extern HWND treeview;
extern int worker_thread_pipe[2];

typedef struct {
    const uint8_t* data;
    uint64_t position;
    uint64_t size;
} ReadableBinaryData;

extern ov_callbacks oggCallbacks;

void SaveBnkOrWpk(HWND window, HTREEITEM rootItem);

void ReplaceWemData(HWND window, HTREEITEM item);

void ExtractSelectedItem(HWND parent, HTREEITEM item);

void* FillProgressBar(void* _args);

uint8_t* WavFromOgg(ReadableBinaryData* oggData);

size_t read_func_callback(void* ptr, size_t size, size_t nmemb, void* datasource);
int seek_func_callback(void* datasource, ogg_int64_t offset, int whence);
long tell_func_callback(void* datasource);

char* rstrstr(const char* haystack, const char* needle);

#endif
