#include <windows.h>
#include <stdio.h>
// #include <stdlib.h>
#include <dwmapi.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#include "resource.h"
#include "templatewindow.h"
#include "list.h"

typedef struct stringWithChildren {
    char* string;
    LIST(struct stringWithChildren) children;
} StringWithChildren;

__declspec(dllimport) StringWithChildren* bnk_extract(int argc, char* argv[]);

#define function void*

void InsertStringToTreeview(HWND Treeview, StringWithChildren* element, HTREEITEM parent)
{
    TVINSERTSTRUCT newItemInfo = {
        .hInsertAfter = TVI_LAST,
        .hParent = parent,
        .item = {
            .mask = TVIF_TEXT,
            .pszText = element->string
        }
    };
    HTREEITEM newItem = (HTREEITEM) SendMessage(Treeview, TVM_INSERTITEM, 0, (LPARAM) &newItemInfo);

    for (uint32_t i = 0; i < element->children.length; i++) {
        InsertStringToTreeview(Treeview, &element->children.objects[i], newItem);
    }
}

const char g_szClassName[] = "myWindowClass";
static HINSTANCE me;
static HWND mainWindow;
static HWND BinTextBox, AudioTextBox, EventsTextBox;
static HWND BinFileSelectButton, AudioFileSelectButton, EventsFileSelectButton, GoButton;
static HWND treeview;

pthread_t worker_thread;
HBRUSH text_background_color;

#define turquoise (RGB(64, 224, 208))
#define dark_turquoise (RGB(0, 206, 209))


// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CTLCOLORSTATIC:
            if (!text_background_color) text_background_color = CreateSolidBrush(turquoise);
            SetBkColor((HDC) wParam, dark_turquoise);
            return (LRESULT) text_background_color;
        case WM_CLOSE: {
            MessageBox(hwnd, "Fenster wird geschlossen...", NULL, MB_ICONQUESTION);
            DestroyWindow(hwnd);
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
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
                        MessageBox(mainWindow, "An error occured while parsing the provided audio files. Most likely you provided none or not the correct files.\nIf there was a log window you could actually read the error message LOL", "Failed to read audio files", MB_ICONERROR);
                    }
                    // proceed to leak all allocated memory here
                    return 0;
                }
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
            }
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
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
    treeview = CreateWindowEx(0, WC_TREEVIEW, NULL, WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT, 0, 80, 450, 400, hwnd, NULL, hInstance, NULL);

    // three text fields for the bin, audio bnk/wpk and events bnk
    BinTextBox = CreateWindowEx(0, "EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER, 115, 10, 450, 20, hwnd, NULL, hInstance, NULL);
    AudioTextBox = CreateWindowEx(0, "EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER, 115, 30, 450, 20, hwnd, NULL, hInstance, NULL);
    EventsTextBox = CreateWindowEx(0, "EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER, 115, 50, 450, 20, hwnd, NULL, hInstance, NULL);

    // buttons yay
    BinFileSelectButton = CreateWindowEx(0, "BUTTON", "select bin file", WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 10, 110, 20, hwnd, NULL, hInstance, NULL);
    AudioFileSelectButton = CreateWindowEx(0, "BUTTON", "select audio file", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 30, 110, 20, hwnd, NULL, hInstance, NULL);
    EventsFileSelectButton = CreateWindowEx(0, "BUTTON", "select events file", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 50, 110, 20, hwnd, NULL, hInstance, NULL);
    GoButton = CreateWindowEx(0, "BUTTON", "Parse files", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 600, 28, 130, 24, hwnd, NULL, hInstance, NULL);
    // disable the ugly selection outline of the text when the button gets pushed
    SendMessage(BinFileSelectButton, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);
    SendMessage(AudioFileSelectButton, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);
    SendMessage(EventsFileSelectButton, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);
    SendMessage(GoButton, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);
    // snatch the nice font from the treeview and slap it onto the textboxes
    // note that there is likely a better way to do this but this works i guess so
    LRESULT fontHandle = SendMessage(treeview, WM_GETFONT, 0, 0);
    SendMessage(BinTextBox, WM_SETFONT, fontHandle, 0);
    SendMessage(AudioTextBox, WM_SETFONT, fontHandle, 0);
    SendMessage(EventsTextBox, WM_SETFONT, fontHandle, 0);


    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
