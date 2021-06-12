#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <vorbis/vorbisfile.h>
#include <windows.h>
#include <shlobj.h>
#include <direct.h>

#include "utility.h"

void ExtractItems(HTREEITEM hItem, wchar_t* output_path)
{
    UINT mask = TVIF_CHILDREN | TVIF_PARAM | TVIF_TEXT;
    if (!TreeView_GetParent(treeview, hItem)) {
        // check whether this is a "global" root item. If so, do not use its (path-like) label text and abuse the fact "//" is equivalent to "/"
        // This should probably be done differently in the future
        mask &= ~TVIF_TEXT;
    }
    char itemText[256] = {0};
    TVITEM tvItem = {
        .mask = mask,
        .hItem = hItem,
        .pszText = itemText,
        .cchTextMax = 255
    };
    TreeView_GetItem(treeview, &tvItem);
    wchar_t current_output_path[wcslen(output_path) + strlen(tvItem.pszText) + 2];
    _swprintf(current_output_path, L"%s/", output_path);
    mbstowcs(current_output_path + wcslen(output_path) + 1, tvItem.pszText, strlen(tvItem.pszText) + 1);

    if (tvItem.lParam) { // item is a child item, has ogg data associated with it
        FILE* output_file = _wfopen(current_output_path, L"wb");
        if (!output_file) {
            MessageBoxW(NULL, L"Failed to open an output file. Which one is still a mystery which needs to be uncovered", current_output_path, MB_ICONWARNING);
            return;
        }
        fwrite(((ReadableBinaryData*) tvItem.lParam)->data, ((ReadableBinaryData*) tvItem.lParam)->size, 1, output_file);
        fclose(output_file);
    } else if (tvItem.cChildren > 0) { // item is a parent item, so extract all children
        // note that cChildren > 0 *should* always be true here
        _wmkdir(current_output_path);
        HTREEITEM child = TreeView_GetChild(treeview, hItem);

        do {
            ExtractItems(child, current_output_path);
        } while ( (child = TreeView_GetNextSibling(treeview, child)) );
    }
}

void ExtractSelectedItem(HWND parent, HTREEITEM item)
{
    printf("item: %p\n", item);
    IFileDialog* pFileDialog;
    // initialize a FileOpenDialog interface
    CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog, (void**) &pFileDialog);
    FILEOPENDIALOGOPTIONS options;
    pFileDialog->lpVtbl->GetOptions(pFileDialog, &options);
    pFileDialog->lpVtbl->SetOptions(pFileDialog, options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM); // make this dialog a folder picker
    if (pFileDialog->lpVtbl->Show(pFileDialog, parent) != S_OK) return; // show the dialog
    IShellItem* selectedItem;
    pFileDialog->lpVtbl->GetResult(pFileDialog, &selectedItem); // get the user-selected item
    pFileDialog->lpVtbl->Release(pFileDialog);
    wchar_t* selectedFolder;
    selectedItem->lpVtbl->GetDisplayName(selectedItem, SIGDN_FILESYSPATH, &selectedFolder); // get the file system path from the selected item
    selectedItem->lpVtbl->Release(selectedItem);
    printf("selected output folder: \"%ls\"\n", selectedFolder);

    ExtractItems(item, selectedFolder);
    CoTaskMemFree(selectedFolder);
}

void* FillProgressBar(void* _args)
{
    HWND progressbar = _args;
    ShowWindow(progressbar, SW_SHOWNORMAL);
    SendMessage(progressbar, PBM_SETRANGE, 0, MAKELONG(0, 5000));
    for (int i = 0; i < 500; i++) {
        SendMessage(progressbar, PBM_STEPIT, 0, 0);
        Sleep(rand() % 100);
    }

    return NULL;
}

uint8_t* WavFromOgg(ReadableBinaryData* oggData)
{
    OggVorbis_File oggDataInfo;
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
