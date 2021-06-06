#include <windows.h>
#include <stdio.h>
// #include <stdlib.h>
#include <dwmapi.h>
#include <math.h>
#include <time.h>

#include "resource.h"

const char g_szClassName[] = "myWindowClass";
static HINSTANCE me;

static HWND edit_text;

int get_number()
{
    static int number = 1;
    number *= 2;
    // number++;
    return number;
}

int windows_created;

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CLOSE: {
            // DestroyWindow(hwnd);
            // char integer[18];
            // sprintf(integer, "%d", get_number());
            // sprintf(integer + 10, "%d", rand());
            // WinMain(NULL, NULL, integer, SW_SHOWNORMAL);
            MessageBox(hwnd, "Fenster wird geschlossen...", NULL, MB_ICONQUESTION);
            break;
        }
        case WM_DESTROY:
            // MessageBox(hwnd, "nice one", "what", 0);
            PostQuitMessage(0);
            break;
        case WM_COMMAND:
            if (lParam && HIWORD(wParam) == BN_CLICKED) {
                char* IQ_text = malloc(100);
                int iq;
                int unlikely = 0;
                while (1) {
                    iq = rand() % 150;
                    if (iq < 70) continue;
                    int distance_from_100 = abs(iq - 100);
                    unlikely += distance_from_100;
                    if (distance_from_100 <= unlikely / 4) break;
                }
                sprintf(IQ_text, "\r\nCalculated IQ: %d", iq);
                SetWindowText(edit_text, IQ_text);
            }
            break;
        case WM_NCHITTEST: {
            UINT nPos = DefWindowProc(hwnd, msg, wParam, lParam);
            if ( nPos == HTCAPTION ) return HTNOWHERE;
            return nPos;
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, __attribute__((unused)) HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    srand(time(NULL));
    me = hInstance;
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    //Step 1: Registering the Window Class

    int icon_size = GetSystemMetrics(SM_CXICON);
    int iconsm_size = GetSystemMetrics(SM_CXSMICON);
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadImage(hInstance, MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, icon_size, icon_size, 0);
    wc.hIconSm       = LoadImage(hInstance, MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, iconsm_size, iconsm_size, 0);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MYMENU);
    wc.lpszClassName = hInstance ? g_szClassName : &lpCmdLine[10];

    if(!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Step 2: Creating the Window
    for (; windows_created < (hInstance ? 1 : strtol(lpCmdLine, NULL, 10)); windows_created++) {
        // int x = ((windows_created / 3) * 480 * 9 / 7) % 1920;
        // int y = (360 * 9 / 7 * windows_created) % 1080;
        int w = 300;
        int h = 200;
        // int x = (425 * windows_created) % 1920;
        // int y = (317 * windows_created) % 1080;
        int x = CW_USEDEFAULT;
        int y = CW_USEDEFAULT;
        RECT rect;
        rect.left   = x;
        rect.top    = y;
        rect.right  = x + w;
        rect.bottom = y + h;
        UINT style = WS_OVERLAPPEDWINDOW ;
        AdjustWindowRectEx(&rect, style, 0, 0);
        // DwmGetWindowAttribute()
        char window_name[30] = "IQ calculator window ";
        sprintf(window_name + 21, "%d", windows_created);
        hwnd = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            g_szClassName,
            window_name,
            style,
            CW_USEDEFAULT, CW_USEDEFAULT, rect.right-rect.left, rect.bottom-rect.top,
            NULL, NULL, hInstance, NULL);
        // AdjustWindowRect(&rect, 0, 0);
        // SetWindowPos(hwnd, NULL, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, 0);

        // button
        CreateWindow(
            "BUTTON",  // Predefined class; Unicode assumed
            "Calculate IQ",      // Button text
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
            100,         // x position
            100,         // y position
            100,        // Button width
            30,        // Button height
            hwnd,     // Parent window
            NULL,       // No menu.
            hInstance,
            NULL
        );      // Pointer not needed.

        // text above the button
        edit_text = CreateWindow(
            "EDIT",
            NULL,
            WS_BORDER | WS_CHILD | WS_VISIBLE | ES_CENTER | ES_READONLY | ES_MULTILINE,
            80,30,140,55,
            hwnd,
            NULL,
            hInstance,
            NULL
        );
        printf("x pos: %d, y pos: %d\n", (windows_created / 3) * 480, (360 * windows_created) % 1080);

        if(hwnd == NULL)
        {
            MessageBox(NULL, "Window Creation Failed!", "Error!",
                MB_ICONEXCLAMATION | MB_OK);
            return 0;
        }

        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }

    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
