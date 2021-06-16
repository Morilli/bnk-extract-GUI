#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>
#include <dwmapi.h>

#include "bnk-extract/api.h"

extern HWND treeview;
extern int worker_thread_pipe[2];

typedef struct {
    const uint8_t* data;
    uint64_t position;
    uint64_t size;
} ReadableBinaryData;

void SaveBnkOrWpk(HWND window, HTREEITEM rootItem);

void ReplaceWemData(HWND window, HTREEITEM item);

void ExtractSelectedItem(HWND parent, HTREEITEM item);

void* FillProgressBar(void* _args);

uint8_t* WavFromOgg(ReadableBinaryData* oggData);

char* rstrstr(const char* haystack, const char* needle);

#endif
