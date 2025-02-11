#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>
#include <dwmapi.h>

extern HWND treeview;
extern int worker_thread_pipe[2];

typedef struct {
    const uint8_t* data;
    uint64_t position;
    uint64_t size;
} ReadableBinaryData;

void SaveBnkOrWpk(HWND window, HTREEITEM rootItem);

void ReplaceWemData(HWND window);

void ExtractSelectedItems(HWND parent);

void* FillProgressBar(void* _args);

uint8_t* WavFromOgg(ReadableBinaryData* oggData);

char* GetPathFromTextBox(HWND textBox);
void AddWemFiles(HWND window, HTREEITEM parentItem);

#endif
