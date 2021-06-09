#include <windows.h>
#include <stdio.h>
#include <dwmapi.h>
#include <time.h>
#include <pthread.h>
#include <vorbis/vorbisfile.h>

#include "alpecin.bin"
#include "spast.bin"

#include "resource.h"
#include "templatewindow.h"
#include "list.h"
#include "utility.h"
#include "api.h"

ReadableBinaryData exampleOgg = {
    .data = spast,
    .size = sizeof(spast)
};

void InsertStringToTreeview(HWND Treeview, StringWithChildren* element, HTREEITEM parent)
{
    // printf("oggData: %p\n", element->oggData->data);
    // printf("lement string: \"%s\"\n", element->string);
    TVINSERTSTRUCT newItemInfo = {
        .hInsertAfter = TVI_LAST,
        .hParent = parent,
        .item = ({TV_ITEM tvItem; if (element->oggData) {
            // printf("oggData: %p\n", element->oggData);
            ReadableBinaryData* data = calloc(1, sizeof(ReadableBinaryData));
            data->data = element->oggData->data;
            data->size = element->oggData->length;
            tvItem = (TV_ITEM) {
                .mask = TVIF_TEXT | TVIF_PARAM,
                .pszText = element->string,
                .lParam = (LPARAM) data
            };
        } else {
            tvItem = (TV_ITEM) {
                .mask = TVIF_TEXT,
                .pszText = element->string
            };
        }
        tvItem; })
    };
    free(element->oggData); // if we're not doing it here... where else would we
    HTREEITEM newItem = TreeView_InsertItem(Treeview, &newItemInfo);

    for (uint32_t i = 0; i < element->children.length; i++) {
        InsertStringToTreeview(Treeview, &element->children.objects[i], newItem);
    }
    free(element->children.objects);
}

// global window variables
const char g_szClassName[] = "myWindowClass";
static HINSTANCE me;
static HWND mainWindow;
static HWND BinTextBox, AudioTextBox, EventsTextBox;
static HWND BinFileSelectButton, AudioFileSelectButton, EventsFileSelectButton, GoButton;
static HWND treeview;
pthread_t worker_thread;


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
        case WM_NOTIFY: {
            // printf("got wm_notify. code is %d\n", ((NMHDR*) lParam)->code);
            if (((NMHDR*) lParam)->code == TVN_SELCHANGED) {
                HTREEITEM selectedItem = ((NMTREEVIEW*) lParam)->itemNew.hItem;
                char filenameBuffer[15] = {0};
                TVITEM treeviewItem = {
                    .mask = TVIF_TEXT | TVIF_PARAM,
                    .pszText = filenameBuffer,
                    .cchTextMax = 14,
                    .hItem = selectedItem
                };
                TreeView_GetItem(treeview, &treeviewItem);
                // printf("cChildren: %d\n", treeviewItem.cChildren);
                // printf("lParam: %p\n", treeviewItem.lParam);
                // printf("text: \"%s\"", treeviewItem.pszText);
                if (treeviewItem.lParam) {
                    // printf("we are a child item\n");
                    ReadableBinaryData* oggData = (ReadableBinaryData*) treeviewItem.lParam;
                    oggData->position = 0;
                    uint8_t* pcmData = WavFromOgg(oggData);
                    if (!pcmData) {
                        MessageBox(hwnd, "sorry your computer has virus", "guten tag", MB_OK | MB_ICONERROR);
                        return 0;
                    } else {
                        // free audio buffer once finished playing
                        PlaySound((char*) pcmData, me, SND_MEMORY | SND_ASYNC);
                    }
                }

                return 0;
            }
            break;
        }
        case WM_CLOSE: {
            MessageBox(hwnd, "Fenster wird geschlossen...", NULL, MB_ICONQUESTION);
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
                    StringWithChildren* structuredWems = bnk_extract(ARRAYSIZE(bnk_extract_args)-1, bnk_extract_args);
                    if (structuredWems)
                        InsertStringToTreeview(treeview, structuredWems, TVI_ROOT);
                    else {
                        MessageBox(mainWindow, "An error occured while parsing the provided audio files. Most likely you provided none or not the correct files.\n"
                        "If there was a log window you could actually read the error message LOL", "Failed to read audio files", MB_ICONERROR);
                    }
                    return 0;
                }
                OggVorbis_File oggDataInfo;
                exampleOgg.position = 0;
                printf("return value: %d\n", ov_open_callbacks(&exampleOgg, &oggDataInfo, NULL, 0, oggCallbacks));
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
                free(rawPcmDataFromOgg);
                // PlaySound(MAKEINTRESOURCE(IN_DER_TAT_SOUND), me, SND_RESOURCE);
                char fileNameBuffer[256] = {0};
                OPENFILENAME fileNameInfo = {
                    .lStructSize = sizeof(OPENFILENAME),
                    .hwndOwner = mainWindow,
                    .lpstrFile = fileNameBuffer,
                    .nMaxFile = 256
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
    // init used common controls
    InitCommonControlsEx(&(INITCOMMONCONTROLSEX) {.dwSize = sizeof(INITCOMMONCONTROLSEX), .dwICC = ICC_PROGRESS_CLASS});
    InitCommonControlsEx(&(INITCOMMONCONTROLSEX) {.dwSize = sizeof(INITCOMMONCONTROLSEX), .dwICC = ICC_TREEVIEW_CLASSES});

    me = hInstance;
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    //Step 1: Registering the Window Class

    int icon_size    = GetSystemMetrics(SM_CXICON);
    int iconsm_size  = GetSystemMetrics(SM_CXSMICON);
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadImage(hInstance, MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, icon_size, icon_size, 0);
    wc.hIconSm       = LoadImage(hInstance, MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, iconsm_size, iconsm_size, 0);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MYMENU);
    wc.lpszClassName = hInstance ? g_szClassName : &lpCmdLine[10];

    if(!RegisterClassEx(&wc))
    {
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
    UINT style = WS_OVERLAPPEDWINDOW ;
    AdjustWindowRectEx(&rect, style, 0, 0);
    char window_name[] = "high quality gui uwu";
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE | WS_EX_LAYERED,
        g_szClassName,
        window_name,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right-rect.left, rect.bottom-rect.top,
        NULL, NULL, hInstance, NULL);
    if(hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    mainWindow = hwnd;
    SetLayeredWindowAttributes(hwnd, 0, 230, LWA_ALPHA);

    // create a tree view
    treeview = CreateWindowEx(0, WC_TREEVIEW, NULL, WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT, 0, 80, 550, 400, hwnd, NULL, hInstance, NULL);

    // three text fields for the bin, audio bnk/wpk and events bnk
    BinTextBox = CreateWindowEx(0, "EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER, 115, 10, 450, 20, hwnd, NULL, hInstance, NULL);
    AudioTextBox = CreateWindowEx(0, "EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER, 115, 31, 450, 20, hwnd, NULL, hInstance, NULL);
    EventsTextBox = CreateWindowEx(0, "EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER, 115, 52, 450, 20, hwnd, NULL, hInstance, NULL);

    // buttons yay
    BinFileSelectButton = CreateWindowEx(0, "BUTTON", "select bin file", WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 05, 9, 110, 22, hwnd, NULL, hInstance, NULL);
    AudioFileSelectButton = CreateWindowEx(0, "BUTTON", "select audio file", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 05, 30, 110, 22, hwnd, NULL, hInstance, NULL);
    EventsFileSelectButton = CreateWindowEx(0, "BUTTON", "select events file", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 05, 51, 110, 22, hwnd, NULL, hInstance, NULL);
    GoButton = CreateWindowEx(0, "BUTTON", "Parse files", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 600, 28, 130, 24, hwnd, NULL, hInstance, NULL);
    // disable the ugly selection outline of the text when the button gets pushed
    SendMessage(BinFileSelectButton, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);
    SendMessage(AudioFileSelectButton, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);
    SendMessage(EventsFileSelectButton, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);
    SendMessage(GoButton, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);


    ShowWindow(hwnd, nCmdShow);
    EnumChildWindows(hwnd, EnumChildProc, 0);
    UpdateWindow(hwnd);

    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
