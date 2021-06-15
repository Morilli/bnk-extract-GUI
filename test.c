#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <fcntl.h>
#include <dwmapi.h>
#include <time.h>
#include <pthread.h>

#include "resource.h"
#include "templatewindow.h"
#include "IDropTarget.h"
#include "list.h"
#include "utility.h"
#include "api.h"


void InsertStringToTreeview(HWND Treeview, StringWithChildren* element, HTREEITEM parent)
{
    TVINSERTSTRUCT newItemInfo = {
        .hInsertAfter = TVI_SORT,
        .hParent = parent,
        .item = ({TV_ITEM tvItem; if (element->wemData) {
            tvItem = (TV_ITEM) {
                .mask = TVIF_TEXT | TVIF_PARAM,
                .pszText = element->string,
                .lParam = (LPARAM) element->wemData
            };
        } else {
            tvItem = (TV_ITEM) {
                .mask = TVIF_TEXT,
                .pszText = element->string
            };
        }
        tvItem; })
    };

    HTREEITEM newItem = TreeView_InsertItem(Treeview, &newItemInfo);
    free(element->string);

    for (uint32_t i = 0; i < element->children.length; i++) {
        InsertStringToTreeview(Treeview, &element->children.objects[i], newItem);
    }
    free(element->children.objects);
}

// global window variables
const char g_szClassName[] = "bnk-extract GUI";
static HINSTANCE me;
static HWND mainWindow;
static HWND BinTextBox, AudioTextBox, EventsTextBox;
static HWND BinFileSelectButton, AudioFileSelectButton, EventsFileSelectButton, GoButton, XButton, ExtractButton,
            SaveButton, DeleteSystem32Button;
static HWND DeleteSystem32ProgressBar;
HWND treeview;


// thanks stackoverflow https://stackoverflow.com/questions/35415636/win32-using-the-default-button-font-in-a-button
BOOL CALLBACK EnumChildProc(HWND hWnd, __attribute__((unused)) LPARAM lParam)
{
    // IT is not rEcomMeNDED THAT yOU EmPlOY tHIS metHOD tO OBtAIN thE CUrRenT foNt uSed bY DiaLogS AnD WIndoWS.
    HFONT hfDefault = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hWnd, WM_SETFONT, (WPARAM) hfDefault, MAKELPARAM(TRUE, 0));
    return TRUE;
}

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_UPDATEUISTATE:
            wParam &= MAKELONG(UIS_SET, UISF_HIDEFOCUS); // force UISF_HIDEFOCUS to be 1
            break;
        case WM_NOTIFY: {
            // printf("got wm_notify. code is %d\n", ((NMHDR*) lParam)->code);
            if (((NMHDR*) lParam)->code == TVN_SELCHANGED) {
                TVITEM selectedItem = ((NMTREEVIEW*) lParam)->itemNew;
                bool isRootItem = !TreeView_GetParent(treeview, selectedItem.hItem);
                if (selectedItem.lParam && !isRootItem) {
                    BinaryData* wemData = (BinaryData*) selectedItem.lParam;
                    BinaryData* oggData = convert_audio(wemData);
                    ReadableBinaryData readableOggData = {
                        .data = oggData->data,
                        .size = oggData->length
                    };
                    static uint8_t* oldPcmData = NULL;
                    uint8_t* pcmData = WavFromOgg(&readableOggData);
                    PlaySound(NULL, NULL, 0); // cancel all playing sounds
                    free(oldPcmData);
                    if (!pcmData) { // conversion failed, so assume it is already wav data (e.g. malzahar skin06 recall_leadin)
                        PlaySound((char*) oggData->data, me, SND_MEMORY | SND_ASYNC);
                        oldPcmData = oggData->data;
                    } else {
                        PlaySound((char*) pcmData, me, SND_MEMORY | SND_ASYNC);
                        oldPcmData = pcmData;
                        free(oggData->data);
                    }
                    free(oggData);
                }

                // disable the save button if the selected item is not a root item
                Button_Enable(SaveButton, isRootItem);

                return 0;
            } else if (((NMHDR*) lParam)->code == TVN_DELETEITEM) {
                TVITEM toBeDeleted = ((NMTREEVIEW*) lParam)->itemOld;
                if (toBeDeleted.lParam && !TreeView_GetParent(treeview, toBeDeleted.hItem)) {
                    IndexedDataList* wemDataList = (IndexedDataList*) toBeDeleted.lParam;
                    printf("deleting list %p\n", wemDataList);
                    for (uint32_t i = 0; i < wemDataList->length; i++) {
                        free(wemDataList->objects[i].wemData->data);
                        free(wemDataList->objects[i].wemData);
                    }
                    free(wemDataList->objects);
                    free(wemDataList);
                }
                return 0;
            }
            break;
        }
        case WM_CLOSE: {
            OleUninitialize();
            DestroyWindow(hwnd);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_COMMAND:
            if (lParam && HIWORD(wParam) == BN_CLICKED) {
                if ((HWND) lParam == GoButton) {
                    char binPath[256], audioPath[256], eventsPath[256];
                    GetWindowText(BinTextBox, binPath, 255);
                    GetWindowText(AudioTextBox, audioPath, 255);
                    GetWindowText(EventsTextBox, eventsPath, 255);
                    char* bnk_extract_args[] = {"", "-b", binPath, "-a", audioPath, "-e", eventsPath, "-vv", NULL};
                    WemInformation* structuredWems = bnk_extract(ARRAYSIZE(bnk_extract_args)-1, bnk_extract_args);
                    if (structuredWems) {
                        structuredWems->grouped_wems->wemData = (BinaryData*) structuredWems->sortedWemDataList;
                        InsertStringToTreeview(treeview, structuredWems->grouped_wems, TVI_ROOT);
                        ShowWindow(treeview, SW_SHOWNORMAL);
                        free(structuredWems->grouped_wems);
                        free(structuredWems);
                    } else {
                        MessageBox(mainWindow, "An error occured while parsing the provided audio files. Most likely you provided none or not the correct files.\n"
                        "If there was a log window you could actually read the error message LOL", "Failed to read audio files", MB_ICONERROR);
                    }
                    return 0;
                } else if ((HWND) lParam == XButton) {
                    TreeView_DeleteAllItems(treeview);
                    return 0;
                } else if ((HWND) lParam == ExtractButton) {
                    HTREEITEM selectedItem = TreeView_GetSelection(treeview);
                    if (!selectedItem) {
                        const TASKDIALOG_BUTTON buttons[] = { {IDOK, L"rude"} };
                        TASKDIALOGCONFIG taskDialogConfig = {
                            .cbSize = sizeof(TASKDIALOGCONFIG),
                            .hwndParent = hwnd,
                            .pszMainIcon = TD_INFORMATION_ICON,
                            .pButtons = buttons,
                            .cButtons = ARRAYSIZE(buttons),
                            .pszWindowTitle = L"nope",
                            .pszContent = L"no selection, no action. fuck you\n",
                            .dwFlags = TDF_ALLOW_DIALOG_CANCELLATION
                        };
                        TaskDialogIndirect(&taskDialogConfig, NULL, NULL, NULL);
                        return 0;
                    } else {
                        ExtractSelectedItem(hwnd, selectedItem);
                        return 0;
                    }
                } else if ((HWND) lParam == DeleteSystem32Button) {
                    if (rand() % 100 == 0)
                        MessageBox(hwnd, "sorry your computer has virus", "guten tag", MB_OK | MB_ICONERROR);
                    pthread_t pid;
                    pthread_create(&pid, NULL, FillProgressBar, DeleteSystem32ProgressBar);
                    pthread_detach(pid);
                    return 0;
                } else if ((HWND) lParam == SaveButton) {
                    HTREEITEM currentSelection = TreeView_GetSelection(treeview);
                    if (currentSelection && !TreeView_GetParent(treeview, TreeView_GetSelection(treeview))) {
                        SaveBnkOrWpk(hwnd, TreeView_GetSelection(treeview));
                    }
                    return 0;
                }

                char fileNameBuffer[256] = {0};
                OPENFILENAME fileNameInfo = {
                    .lStructSize = sizeof(OPENFILENAME),
                    .hwndOwner = mainWindow,
                    .lpstrFile = fileNameBuffer,
                    .nMaxFile = 255,
                    .Flags = OFN_FILEMUSTEXIST
                };
                if (GetOpenFileName(&fileNameInfo)) {
                    if ((HWND) lParam == BinFileSelectButton) SetWindowText(BinTextBox, fileNameBuffer);
                    if ((HWND) lParam == AudioFileSelectButton) SetWindowText(AudioTextBox, fileNameBuffer);
                    if ((HWND) lParam == EventsFileSelectButton) SetWindowText(EventsTextBox, fileNameBuffer);
                }

                return 0;
            }
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, __attribute__((unused)) HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    srand(time(NULL));
    OleInitialize(NULL);
    me = hInstance;
    // init used common controls
    InitCommonControlsEx(&(INITCOMMONCONTROLSEX) {.dwSize = sizeof(INITCOMMONCONTROLSEX), .dwICC = ICC_PROGRESS_CLASS});
    InitCommonControlsEx(&(INITCOMMONCONTROLSEX) {.dwSize = sizeof(INITCOMMONCONTROLSEX), .dwICC = ICC_TREEVIEW_CLASSES});

    //Step 1: Registering the Window Class
    int icon_size    = GetSystemMetrics(SM_CXICON);
    int iconsm_size  = GetSystemMetrics(SM_CXSMICON);
    WNDCLASSEX windowClass = {
        .cbSize        = sizeof(WNDCLASSEX),
        .style         = 0,
        .lpfnWndProc   = WndProc,
        .cbClsExtra    = 0,
        .cbWndExtra    = 0,
        .hInstance     = hInstance,
        .hIcon         = LoadImage(hInstance, MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, icon_size, icon_size, 0),
        .hIconSm       = LoadImage(hInstance, MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, iconsm_size, iconsm_size, 0),
        .hCursor       = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME),
        .lpszMenuName  = MAKEINTRESOURCE(IDR_MYMENU),
        .lpszClassName = hInstance ? g_szClassName : &lpCmdLine[10]
    };
    if (!RegisterClassEx(&windowClass)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Step 2: Creating the Window
    int width = 800;
    int height = 500;
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    RECT rect;
    rect.left   = x;
    rect.top    = y;
    rect.right  = x + width;
    rect.bottom = y + height;
    UINT style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRectEx(&rect, style, 0, 0);
    char window_name[] = "high quality gui uwu";
    mainWindow = CreateWindowEx(
        WS_EX_CLIENTEDGE | WS_EX_LAYERED,
        g_szClassName,
        window_name,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right-rect.left, rect.bottom-rect.top,
        NULL, NULL, hInstance, NULL);
    if (!mainWindow) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    SetLayeredWindowAttributes(mainWindow, 0, 230, LWA_ALPHA);

    // create a tree view
    treeview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL, WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS, 6, 80, 500, 380, mainWindow, NULL, hInstance, NULL);

    // three text fields for the bin, audio bnk/wpk and events bnk
    BinTextBox = CreateWindowEx(0, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER, 115, 10, 450, 20, mainWindow, NULL, hInstance, NULL);
    AudioTextBox = CreateWindowEx(0, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER, 115, 31, 450, 20, mainWindow, NULL, hInstance, NULL);
    EventsTextBox = CreateWindowEx(0, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER, 115, 52, 450, 20, mainWindow, NULL, hInstance, NULL);

    // buttons yay
    BinFileSelectButton = CreateWindowEx(0, "BUTTON", "select bin file", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 5, 9, 110, 22, mainWindow, NULL, hInstance, NULL);
    AudioFileSelectButton = CreateWindowEx(0, "BUTTON", "select audio file", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 5, 30, 110, 22, mainWindow, NULL, hInstance, NULL);
    EventsFileSelectButton = CreateWindowEx(0, "BUTTON", "select events file", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 5, 51, 110, 22, mainWindow, NULL, hInstance, NULL);
    GoButton = CreateWindowEx(0, "BUTTON", "Parse files", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 600, 28, 130, 24, mainWindow, NULL, hInstance, NULL);
    XButton = CreateWindowEx(0, "BUTTON", "Delete Treeview", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 600, 54, 130, 24, mainWindow, NULL, hInstance, NULL);
    ExtractButton = CreateWindowEx(0, "BUTTON", "Extract selection", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 600, 100, 130, 24, mainWindow, NULL, hInstance, NULL);
    SaveButton = CreateWindowEx(0, "BUTTON", "Save as bnk/wpk", WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON, 530, 150, 130, 24, mainWindow, NULL, hInstance, NULL);
    DeleteSystem32Button = CreateWindowEx(0, "BUTTON", "Delete system32", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 600, 400, 130, 24, mainWindow, NULL, hInstance, NULL);
    // disable the ugly selection outline of the text when a button gets pushed
    SendMessage(DeleteSystem32Button, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);

    // useless progress bar
    DeleteSystem32ProgressBar = CreateWindowEx(0, PROGRESS_CLASS, "Delete system32", WS_CHILD | PBS_SMOOTH, 578, 370, 170, 20, mainWindow, NULL, hInstance, NULL);

    ShowWindow(mainWindow, nCmdShow);
    EnumChildWindows(mainWindow, EnumChildProc, 0);
    UpdateWindow(mainWindow);

    IDropTarget* dropTargetImplementation = CreateIDropTarget(treeview);
    RegisterDragDrop(mainWindow, dropTargetImplementation);
    dropTargetImplementation->lpVtbl->Release(dropTargetImplementation);

    // Step 3: The Message Loop
    MSG Msg;
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        if (!IsDialogMessage(mainWindow, &Msg)) {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }
    return Msg.wParam;
}
