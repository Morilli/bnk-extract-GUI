#include <windows.h>
#include <dwmapi.h>
#include <stdio.h>
#include <stdbool.h>
#include <shlobj.h>
#include "utility.h"

typedef struct InternalIDropTarget {
    IDropTargetVtbl* _vTable;
    UINT reference_counter;
    bool dragValid;
    HWND window;
} InternalIDropTarget;

static HRESULT STDMETHODCALLTYPE QueryInterface(IDropTarget* This, REFIID riid, void** ppvObject)
{
    if (!ppvObject) {
        return E_POINTER;
    }
    if (riid == &IID_IUnknown || riid == &IID_IDropTarget) {
        *ppvObject = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE AddRef(IDropTarget* This)
{
    InternalIDropTarget* idropTarget = (InternalIDropTarget*) This;
    idropTarget->reference_counter++;

    return idropTarget->reference_counter;
}

static ULONG STDMETHODCALLTYPE Release(IDropTarget* This)
{
    InternalIDropTarget* idropTarget = (InternalIDropTarget*) This;
    idropTarget->reference_counter--;
    UINT reference_counter = idropTarget->reference_counter;
    if (idropTarget->reference_counter == 0) {
        free(idropTarget->_vTable);
        free(idropTarget);
    }

    return reference_counter;
}

static HRESULT STDMETHODCALLTYPE DragEnter(IDropTarget* This, IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    if (!pdwEffect) return E_INVALIDARG;
    (void) grfKeyState;
    (void) pt;
    InternalIDropTarget* idropTarget = (InternalIDropTarget*) This;
    *pdwEffect = DROPEFFECT_NONE;

    printf("got into dragenter\n");
    IEnumFORMATETC* enumFormatEtc;
    if (pDataObj->lpVtbl->EnumFormatEtc(pDataObj, DATADIR_GET, &enumFormatEtc) == S_OK) {
        FORMATETC formatEtc;
        while (enumFormatEtc->lpVtbl->Next(enumFormatEtc, 1, &formatEtc, NULL) == S_OK) {
            CoTaskMemFree(formatEtc.ptd);
            if (formatEtc.cfFormat == CF_HDROP) {
                idropTarget->dragValid = true;
                STGMEDIUM stgMedium;
                printf("return value of pDataObj->GetData: %ld\n", pDataObj->lpVtbl->GetData(pDataObj, &formatEtc, &stgMedium));
                HDROP hDropInfo = (HDROP)(DROPFILES*) stgMedium.hGlobal;
                UINT nFiles = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
                printf("nFiles: %u\n", nFiles);
                for (UINT index = 0; index < nFiles; index++) {
                    UINT fileNameSize = DragQueryFile(hDropInfo, index, NULL, 0);
                    char fileNameBuffer[fileNameSize + 1];
                    DragQueryFile(hDropInfo, index, fileNameBuffer, fileNameSize + 1);
                    if (fileNameSize < 4 || memcmp(&fileNameBuffer[fileNameSize-4], ".wem", 4) != 0) {
                        idropTarget->dragValid = false;
                        break;
                    }
                }
                ReleaseStgMedium(&stgMedium);
                break;
            }
        }
        enumFormatEtc->lpVtbl->Release(enumFormatEtc);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DragOver(IDropTarget* This, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    if (!pdwEffect) return E_INVALIDARG;
    (void) grfKeyState;
    InternalIDropTarget* idropTarget = (InternalIDropTarget*) This;
    *pdwEffect = DROPEFFECT_NONE;

    // early return in case DragEnter determined that this data is not drag-drop supported
    if (!idropTarget->dragValid) return S_OK;
    printf("x: %ld, y: %ld\n", pt.x, pt.y);
    // The POINT structure is identical to the POINTL structure.
    POINT point = {
        .x = pt.x,
        .y = pt.y
    };
    printf("screentoclient return: %d\n", ScreenToClient(treeview, &point));
    printf("x: %ld, y: %ld\n", point.x, point.y);
    TVHITTESTINFO hitTestInfo = {.pt = point};
    HTREEITEM hoveredItem = TreeView_HitTest(treeview, &hitTestInfo);
    printf("hovered Item: %p\n", hoveredItem);
    if (hoveredItem) *pdwEffect = DROPEFFECT_COPY;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DragLeave(IDropTarget *This)
{
    InternalIDropTarget* idropTarget = (InternalIDropTarget*) This;
    idropTarget->dragValid = false;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Drop(IDropTarget* This, IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    if (!pdwEffect) return E_INVALIDARG;
    (void) grfKeyState;
    InternalIDropTarget* idropTarget = (InternalIDropTarget*) This;
    *pdwEffect = DROPEFFECT_COPY;

    printf("got into drop\n");
    POINT point = {
        .x = pt.x,
        .y = pt.y
    };
    ScreenToClient(treeview, &point);
    TVHITTESTINFO hitTestInfo = {.pt = point};
    HTREEITEM currentItem = TreeView_HitTest(treeview, &hitTestInfo);
    printf("current item: %p\n", currentItem);
    HTREEITEM rootItem = currentItem;
    while ( (currentItem = TreeView_GetParent(treeview, currentItem)) ) {
        rootItem = currentItem;
    }
    printf("root item: %p, current item: %p\n", rootItem, currentItem);
    TVITEM tvItem = {
        .mask = TVIF_PARAM,
        .hItem = rootItem
    };
    TreeView_GetItem(treeview, &tvItem);
    IndexedDataList* wemDataList = (IndexedDataList*) tvItem.lParam;

    IEnumFORMATETC* enumFormatEtc;
    if (pDataObj->lpVtbl->EnumFormatEtc(pDataObj, DATADIR_GET, &enumFormatEtc) == S_OK) {
        FORMATETC formatEtc;
        while (enumFormatEtc->lpVtbl->Next(enumFormatEtc, 1, &formatEtc, NULL) == S_OK) {
            CoTaskMemFree(formatEtc.ptd);
            if (formatEtc.cfFormat == CF_HDROP) {
                STGMEDIUM stgMedium;
                if (pDataObj->lpVtbl->GetData(pDataObj, &formatEtc, &stgMedium) != S_OK)
                    continue;
                HDROP hDropInfo = (HDROP)(DROPFILES*) stgMedium.hGlobal;
                UINT nFiles = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
                printf("nFiles: %u\n", nFiles);
                for (UINT index = 0; index < nFiles; index++) {
                    UINT fileNameSize = DragQueryFile(hDropInfo, index, NULL, 0);
                    char fileNameBuffer[fileNameSize + 1];
                    DragQueryFile(hDropInfo, index, fileNameBuffer, fileNameSize + 1);
                    uint32_t current_file_id = strtol(rstrstr(fileNameBuffer, "\\") + 1, NULL, 0);
                    IndexedData* wemData = NULL;
                    printf("wemDataList: %p\n", wemDataList);
                    printf("length of list: %u, %u\n", wemDataList->length, wemDataList->allocated_length);
                    for (uint32_t i = 0; i < wemDataList->length; i++) {
                        printf("objects[%d]: %u\n", i, wemDataList->objects[i].id);
                    }
                    find_object_s(wemDataList, wemData, id, current_file_id);
                    printf("wemData: %p\n", wemData);
                    if (wemData) {
                        FILE* newDataFile = fopen(fileNameBuffer, "rb");
                        if (!newDataFile) continue;
                        fseek(newDataFile, 0, SEEK_END);
                        wemData->wemData->length = ftell(newDataFile);
                        free(wemData->wemData->data);
                        wemData->wemData->data = malloc(wemData->wemData->length);
                        rewind(newDataFile);
                        fread(wemData->wemData->data, wemData->wemData->length, 1, newDataFile);
                        fclose(newDataFile);
                    }
                }
                ReleaseStgMedium(&stgMedium);
                break;
            }
        }
        enumFormatEtc->lpVtbl->Release(enumFormatEtc);
    }
    idropTarget->dragValid = false;

    return S_OK;
}


IDropTarget* CreateIDropTarget(HWND window)
{
    InternalIDropTarget* newIDropTarget = malloc(sizeof(InternalIDropTarget));
    newIDropTarget->_vTable = malloc(sizeof(IDropTargetVtbl));
    newIDropTarget->_vTable->QueryInterface = QueryInterface;
    newIDropTarget->_vTable->AddRef = AddRef;
    newIDropTarget->_vTable->Release = Release;
    newIDropTarget->_vTable->DragEnter = DragEnter;
    newIDropTarget->_vTable->DragOver = DragOver;
    newIDropTarget->_vTable->DragLeave = DragLeave;
    newIDropTarget->_vTable->Drop = Drop;
    newIDropTarget->reference_counter = 1;
    newIDropTarget->dragValid = false;
    newIDropTarget->window = window;

    return (IDropTarget*) newIDropTarget;
}
