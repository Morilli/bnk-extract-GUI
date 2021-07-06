#define OV_EXCLUDE_STATIC_CALLBACKS
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <windows.h>
#include <shlobj.h>
#include <direct.h>
#include <dwmapi.h>

#include "utility.h"
#include "settings.h"
#include "treeview_extension.h"
#include "bnk-extract/api.h"
#include "vorbis/vorbisfile.h"

static size_t read_func_callback(void* ptr, size_t size, size_t nmemb, void* datasource)
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

static int seek_func_callback(void* datasource, ogg_int64_t offset, int whence)
{
    ReadableBinaryData* source = datasource;

    ssize_t requested_position = offset;
    if (whence == SEEK_CUR) requested_position += source->position;
    if (whence == SEEK_END) requested_position += source->size;
    if ((size_t) requested_position > source->size || requested_position < 0) return -1;

    source->position = requested_position;
    return 0;
}

static long tell_func_callback(void* datasource)
{
    ReadableBinaryData* source = datasource;

    return source->position;
}

static const ov_callbacks oggCallbacks = {
    .read_func = read_func_callback,
    .seek_func = seek_func_callback,
    .tell_func = tell_func_callback,
    .close_func = NULL
};

// note: ONLY DO THIS WITH POWERS OF 2
// clamps number to the next higher number that devides through this power of two, e.g. (1234, 8) -> 1240
#define clamp_int(number, clamp) (((clamp-1) + number) & ~(clamp-1))
// gives the distance to the next higher number that devides through this power of two, e.g. (1234, 8) -> 6
#define diff_to_clamp(number, clamp) ((clamp-1) - ((number+clamp-1) & (clamp-1)))
// gives the distance to the next lower number that devides through this power of two, e.g. (1234, 8) -> 2
#define diff_from_clamp(number, clamp) ((number+clamp-1) & (clamp-1))

static void write_wpk_file(AudioDataList* wemFiles, char* outputPath)
{
    const int clamp = 8;
    FILE* wpk_file = fopen(outputPath, "wb");
    if (!wpk_file) {
        MessageBox(NULL, "Failed to open some wpk output file", outputPath, MB_ICONERROR);
        return;
    }

    // write header
    fwrite("r3d2\1\0\0\0", 8, 1, wpk_file);
    fwrite(&wemFiles->length, 4, 1, wpk_file);

    // skip over the initial offset section and write them later
    fseek(wpk_file, wemFiles->length * 4, SEEK_CUR);
    fseek(wpk_file, diff_to_clamp(ftell(wpk_file), clamp), SEEK_CUR);

    char filename_string[15];
    uint32_list offset_list;
    initialize_list_size(&offset_list, wemFiles->length);
    for (uint32_t i = 0; i < wemFiles->length; i++) {
        // save offset to fill in the offset section later
        add_object(&offset_list, &(uint32_t) {ftell(wpk_file)});
        fseek(wpk_file, 4, SEEK_CUR);
        fwrite(&wemFiles->objects[i].length, 4, 1, wpk_file);
        sprintf(filename_string, "%u.wem", wemFiles->objects[i].id);
        int filename_string_length = strlen(filename_string);
        fwrite(&filename_string_length, 4, 1, wpk_file);
        for (int i = 0; i < filename_string_length; i++) {
            putc(filename_string[i], wpk_file);
            fseek(wpk_file, 1, SEEK_CUR);
        }
        fseek(wpk_file, diff_to_clamp(ftell(wpk_file), clamp), SEEK_CUR);
    }

    uint32_t start_data_offset = ftell(wpk_file);
    for (uint32_t i = 0; i < wemFiles->length; i++) {
        // seek to initial place in the offset section and write offset
        fseek(wpk_file, 12 + 4*i, SEEK_SET);
        fwrite(&offset_list.objects[i], 4, 1, wpk_file);

        // seek to the written offset, update the offset variable for further use and write data offset
        fseek(wpk_file, offset_list.objects[i], SEEK_SET);
        offset_list.objects[i] = i == 0 ? start_data_offset : clamp_int(offset_list.objects[i-1] + wemFiles->objects[i-1].length, clamp);
        fwrite(&offset_list.objects[i], 4, 1, wpk_file);

        // seek to written data offset and write data
        fseek(wpk_file, offset_list.objects[i], SEEK_SET);
        fwrite(wemFiles->objects[i].data, wemFiles->objects[i].length, 1, wpk_file);
    }

    fclose(wpk_file);
    free(offset_list.objects);
}

static void write_bnk_file(AudioDataList* wemFiles, char* outputPath)
{
    const int clamp = 16;
    const uint32_t version = 0x86; // TODO FIXME this should be taken from the original source file
    const uint32_t bkhd_section_length = 0x14; // this as well
    FILE* bnk_file = fopen(outputPath, "wb");
    if (!bnk_file) {
        MessageBox(NULL, "Failed to open some bnk output file", outputPath, MB_ICONERROR);
        return;
    }

    // write BKHD section
    fwrite("BKHD", 4, 1, bnk_file);
    uint8_t hardcoded[12] = "\0\0\0\0\xfa\0\0\0\0\0\0\0";
    fwrite(&bkhd_section_length, 4, 1, bnk_file);
    fwrite(&version, 4, 1, bnk_file); // version
    fwrite("\x00\x00\x00\x00", 4, 1, bnk_file); // TODO FIXME id of this bnk file, should ideally be given or taken from the original file
    fwrite("\x3e\x5d\x70\x17", 4, 1, bnk_file); // random hardcoded bytes?
    fwrite(hardcoded, bkhd_section_length - 12, 1, bnk_file);

    // write DIDX section
    fwrite("DIDX", 4, 1, bnk_file);
    fwrite(&(uint32_t) {wemFiles->length*12}, 4, 1, bnk_file);
    uint32_t initial_clamp_offset = ftell(bnk_file) + wemFiles->length*12 + 8;
    uint32_list offset_list;
    initialize_list_size(&offset_list, wemFiles->length);
    for (uint32_t i = 0; i < wemFiles->length; i++) {
        offset_list.objects[i] = i == 0 ? 0 : offset_list.objects[i-1] + wemFiles->objects[i-1].length;
        offset_list.objects[i] += diff_to_clamp(offset_list.objects[i] + initial_clamp_offset, clamp);
        fwrite(&wemFiles->objects[i].id, 4, 1, bnk_file);
        fwrite(&offset_list.objects[i], 4, 1, bnk_file);
        fwrite(&wemFiles->objects[i].length, 4, 1, bnk_file);
    }

    // write DATA section
    fwrite("DATA", 4, 1, bnk_file);
    fwrite(&(uint32_t) {offset_list.objects[wemFiles->length-1] + wemFiles->objects[wemFiles->length-1].length}, 4, 1, bnk_file);
    for (uint32_t i = 0; i < wemFiles->length; i++) {
        fseek(bnk_file, diff_to_clamp(ftell(bnk_file), clamp), SEEK_CUR);
        fwrite(wemFiles->objects[i].data, wemFiles->objects[i].length, 1, bnk_file);
    }

    fclose(bnk_file);
    free(offset_list.objects);
}


void SaveBnkOrWpk(HWND window, HTREEITEM root)
{
    char itemText[256] = {0};
    TVITEM tvItem = {
        .mask = TVIF_PARAM | TVIF_TEXT,
        .hItem = root,
        .pszText = itemText,
        .cchTextMax = 255
    };
    TreeView_GetItem(treeview, &tvItem);

    OPENFILENAME fileNameInfo = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner = window,
        .lpstrFile = tvItem.pszText,
        .nMaxFile = 255,
        .lpstrFilter = "Audio files\0*.bnk;*.wpk\0All files\0*.*\0\0",
        .Flags = OFN_OVERWRITEPROMPT
    };
    if (GetSaveFileName(&fileNameInfo)) {
        char* selectedFile = fileNameInfo.lpstrFile;
        printf("selected file: \"%s\"\n", selectedFile);
        if (strstr(selectedFile, ".wpk")) {
            write_wpk_file((AudioDataList*) tvItem.lParam, selectedFile);
        } else {
            write_bnk_file((AudioDataList*) tvItem.lParam, selectedFile);
        }
        // TODO check if .wpk or .bnk was selected, and do something if neither was
    }
}

void ReplaceWemData(HWND window)
{
    if (!TreeView_GetSelectedCount(treeview)) return;

    LIST(AudioData*) selectedChildItemsDataList;
    initialize_list(&selectedChildItemsDataList);
    HTREEITEM currentItem = NULL;
    while ( (currentItem = TreeView_GetNextSelected(treeview, currentItem)) ) {
        printf("current item here: %p\n", currentItem);
        TVITEM tvItem = {
            .mask = TVIF_PARAM,
            .hItem = currentItem
        };
        TreeView_GetItem(treeview, &tvItem);
        if (tvItem.lParam && !TreeView_IsRootItem(currentItem)) { // add child items only
            bool found = false;
            for (uint32_t i = 0; i < selectedChildItemsDataList.length; i++) {
                if (selectedChildItemsDataList.objects[i] == (AudioData*) tvItem.lParam)
                    found = true; // the same audio files can appear in the treeview multiple times, so don't use them multiple times
            }
            if (!found) add_object(&selectedChildItemsDataList, &(AudioData*) {(AudioData*) tvItem.lParam});
        }
    }

    char* fileNameBuffer = malloc(UINT16_MAX);
    fileNameBuffer[0] = '\0';
    OPENFILENAME fileNameInfo = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner = window,
        .lpstrFile = fileNameBuffer,
        .nMaxFile = UINT16_MAX,
        .Flags = OFN_FILEMUSTEXIST,
        .lpstrFilter = "Wem audio files\0*.wem\0All files\0*.*\0\0"
    };
    if (selectedChildItemsDataList.length > 1)
        fileNameInfo.Flags |= OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    if (GetOpenFileName(&fileNameInfo)) {
        uint32_t nFilesSelected = 0;
        const char* c = fileNameInfo.lpstrFile;
        if (selectedChildItemsDataList.length > 1)
        while (*c || *(c+1)) {
            if (!*c) nFilesSelected++;
            c++;
        }
        if (!nFilesSelected)
            nFilesSelected = 1;
        printf("nFilesSelected: %d\n", nFilesSelected);
        if (nFilesSelected > selectedChildItemsDataList.length) {
            char warningMessage[sizeof("Warning: 123456789A files selected, but only 123456789A are going to be used")];
            sprintf(warningMessage, "Warning: %d files selected, but only %d are going to be used", nFilesSelected, selectedChildItemsDataList.length);
            MessageBox(window, warningMessage, "Too many files selected!", MB_ICONWARNING);
        } else if (nFilesSelected > 1) {
            char infoMessage[sizeof("Info: Replacing 123456789A files randomly with 123456789A")];
            sprintf(infoMessage, "Info: Replacing %d files randomly with %d", selectedChildItemsDataList.length, nFilesSelected);
            MessageBox(window, infoMessage, "a", 0);
        }
        const char* currentPosition;
        for (uint32_t i = 0; i < selectedChildItemsDataList.length; i++) {
            if (i % nFilesSelected == 0)
                currentPosition = fileNameInfo.lpstrFile;
            currentPosition += strlen(currentPosition) + 1;
            char currentFileName[PATH_MAX];
            sprintf(currentFileName, "%s\\%s", fileNameInfo.lpstrFile, currentPosition);
            printf("current file name: \"%s\"\n", nFilesSelected == 1 ? fileNameInfo.lpstrFile : currentFileName);
            FILE* newDataFile = fopen(nFilesSelected == 1 ? fileNameInfo.lpstrFile : currentFileName, "rb");
            if (!newDataFile) {
                if (nFilesSelected == 1) break;
                char errorMessage[sizeof("Failed to open file \"\"!\nReplacement will be inconsistent.") + strlen(currentFileName)];
                sprintf(errorMessage, "Failed to open file \"%s\"!\nReplacement will be inconsistent.", currentFileName);
                MessageBox(window, errorMessage, "Failed to open wem file", MB_ICONERROR);
                continue;
            }
            fseek(newDataFile, 0, SEEK_END);
            selectedChildItemsDataList.objects[i]->length = ftell(newDataFile);
            free(selectedChildItemsDataList.objects[i]->data);
            selectedChildItemsDataList.objects[i]->data = malloc(selectedChildItemsDataList.objects[i]->length);
            rewind(newDataFile);
            fread(selectedChildItemsDataList.objects[i]->data, selectedChildItemsDataList.objects[i]->length, 1, newDataFile);
            fclose(newDataFile);
        }
    }
    free(fileNameBuffer);
    free(selectedChildItemsDataList.objects);
}

static void ExtractItems(HTREEITEM hItem, wchar_t* output_path)
{
    // check whether this is a "global" root item. If so, do not use its (path-like) label text and abuse the fact "//" is equivalent to "/"
    bool isRootItem = TreeView_IsRootItem(hItem);
    UINT mask = TVIF_CHILDREN | TVIF_PARAM | TVIF_TEXT;
    if (isRootItem)
        mask &= ~TVIF_TEXT;
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

    if (tvItem.lParam && !isRootItem) { // item is a child item, has wem data associated with it
        if (settings[ID_EXTRACT_AS_WEM-SETTINGS_OFFSET]) { // should be extracted as wem
            FILE* output_file = _wfopen(current_output_path, L"wb");
            if (!output_file) {
                MessageBoxW(NULL, L"Failed to open a wem output file. Which one is still a mystery which needs to be uncovered", current_output_path, MB_ICONWARNING);
                return;
            }
            AudioData* wemData = (AudioData*) tvItem.lParam;
            fwrite(wemData->data, wemData->length, 1, output_file);
            fclose(output_file);
        }

        if (settings[ID_EXTRACT_AS_OGG-SETTINGS_OFFSET]) { // should be extracted as ogg
            AudioData* wemData = (AudioData*) tvItem.lParam;
            BinaryData* oggData = WemToOgg(wemData);
            if (oggData) { // some rare wem files fail to convert. Just ignore them silently here; it's not worth it
                if (oggData->length >= 4 && memcmp(oggData->data, "RIFF", 4) == 0) // it's actually wav data
                    _swprintf(current_output_path + wcslen(current_output_path) - 3, L"wav");
                else
                    _swprintf(current_output_path + wcslen(current_output_path) - 3, L"ogg");
                FILE* output_file = _wfopen(current_output_path, L"wb");
                if (!output_file) {
                    MessageBoxW(NULL, L"Failed to open an ogg output file. Which one is still a mystery which needs to be uncovered", current_output_path, MB_ICONWARNING);
                    return;
                }
                fwrite(oggData->data, oggData->length, 1, output_file);
                fclose(output_file);
                free(oggData->data);
                free(oggData);
            }
        }
    } else if (tvItem.cChildren > 0) { // item is a parent item, so extract all children
        // note that cChildren > 0 *should* always be true here
        _wmkdir(current_output_path);
        HTREEITEM child = TreeView_GetChild(treeview, hItem);

        do {
            ExtractItems(child, current_output_path);
        } while ( (child = TreeView_GetNextSibling(treeview, child)) );
    }
}

static wchar_t* GetOpenFolderName(HWND parent)
{
    IFileDialog* pFileDialog;
    // initialize a FileOpenDialog interface
    CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog, (void**) &pFileDialog);
    FILEOPENDIALOGOPTIONS options;
    pFileDialog->lpVtbl->GetOptions(pFileDialog, &options);
    pFileDialog->lpVtbl->SetOptions(pFileDialog, options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM); // make this dialog a folder picker
    if (pFileDialog->lpVtbl->Show(pFileDialog, parent) != S_OK) return NULL; // show the dialog
    IShellItem* selectedItem;
    pFileDialog->lpVtbl->GetResult(pFileDialog, &selectedItem); // get the user-selected item (folder)
    pFileDialog->lpVtbl->Release(pFileDialog);
    wchar_t* selectedFolder;
    selectedItem->lpVtbl->GetDisplayName(selectedItem, SIGDN_FILESYSPATH, &selectedFolder); // get the file system path from the selected item
    selectedItem->lpVtbl->Release(selectedItem);

    return selectedFolder;
}

void ExtractSelectedItems(HWND parent)
{
    wchar_t* selectedFolder = GetOpenFolderName(parent);
    if (!selectedFolder) return;
    printf("selected output folder: \"%ls\"\n", selectedFolder);

    HTREEITEM selectedItem = NULL;
    while ( (selectedItem = TreeView_GetNextSelected(treeview, selectedItem)) ) {
        ExtractItems(selectedItem, selectedFolder);
    }

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
